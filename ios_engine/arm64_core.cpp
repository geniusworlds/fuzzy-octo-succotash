#include "arm64_core.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

Arm64Core::Arm64Core(std::shared_ptr<DarwinRuntime> rt) : runtime(rt), halted(false) {
    reset();
}

Arm64Core::~Arm64Core() {}

void Arm64Core::reset() {
    std::memset(&regs, 0, sizeof(regs));
    halted = false;
}

void Arm64Core::handleSyscall(uint32_t syscallNum) {
    std::ostringstream ss;
    ss << "Syscall Trap invoked: 0x" << std::hex << syscallNum << std::dec;
    
    switch (syscallNum) {
        case 1: // exit(status)
            ss << " (SYS_exit) with code " << regs.x[0];
            runtime->log(ss.str());
            halted = true;
            break;
        case 4: { // write(fd, buf, count)
            uint64_t fd = regs.x[0];
            uint64_t bufAddr = regs.x[1];
            uint64_t count = regs.x[2];
            std::vector<char> strBuf(count + 1, 0);
            if (runtime->readMemory(bufAddr, strBuf.data(), count)) {
                runtime->log("[App Output]: " + std::string(strBuf.data(), count));
                regs.x[0] = count; // return bytes written
            } else {
                regs.x[0] = -1; // EFAULT
            }
            break;
        }
        case 20: // getpid()
            regs.x[0] = 42; // Virtualized iOS PID
            break;
        case 197: { // mmap
            uint64_t size = regs.x[1];
            uint64_t alloc = runtime->mach_vm_allocate(size);
            regs.x[0] = alloc;
            break;
        }
        default:
            ss << " (Unimplemented Darwin syscall)";
            runtime->log(ss.str());
            regs.x[0] = 0;
            break;
    }
}

bool Arm64Core::step() {
    if (halted) return false;

    uint32_t instruction = 0;
    if (!runtime->readMemory(regs.pc, &instruction, sizeof(instruction))) {
        // Halt if execution goes beyond mapped virtual text segment
        return false;
    }

    // Advance PC
    regs.pc += 4;

    // Decode basic ARM64 instruction patterns
    // SVC instruction: 1101 0100 000 imm16 000 01 (0xD4000001)
    if ((instruction & 0xFFE0001F) == 0xD4000001) {
        uint32_t imm16 = (instruction >> 5) & 0xFFFF;
        // Darwin convention: X16 contains syscall number or SVC immediate
        uint32_t syscallNum = (regs.x[16] != 0) ? regs.x[16] : imm16;
        handleSyscall(syscallNum);
        return true;
    }

    // RET (0xD65F03C0)
    if (instruction == 0xD65F03C0) {
        regs.pc = regs.x[30]; // LR
        return true;
    }

    // NOP (0xD503201F)
    if (instruction == 0xD503201F) {
        return true;
    }

    return true;
}

bool Arm64Core::executeInstructions(size_t maxCount) {
    size_t executed = 0;
    while (!halted && executed < maxCount) {
        if (!step()) break;
        executed++;
    }
    return executed > 0;
}
