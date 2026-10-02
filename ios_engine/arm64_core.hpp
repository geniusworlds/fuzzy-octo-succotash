#ifndef ARM64_CORE_HPP
#define ARM64_CORE_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "darwin_runtime.hpp"

struct Arm64Registers {
    uint64_t x[31]; // X0-X30
    uint64_t sp;    // Stack Pointer
    uint64_t pc;    // Program Counter
    uint32_t cpsr;  // NZCV flags
};

class Arm64Core {
public:
    Arm64Core(std::shared_ptr<DarwinRuntime> runtime);
    ~Arm64Core();

    void reset();
    void setPC(uint64_t entryPoint) { regs.pc = entryPoint; }
    void setSP(uint64_t stackTop) { regs.sp = stackTop; }

    Arm64Registers& getRegisters() { return regs; }
    const Arm64Registers& getRegisters() const { return regs; }

    bool step();
    bool executeInstructions(size_t maxCount);

    // Syscall Handler for Darwin/XNU
    void handleSyscall(uint32_t syscallNum);

private:
    Arm64Registers regs;
    std::shared_ptr<DarwinRuntime> runtime;
    bool halted;
};

#endif // ARM64_CORE_HPP
