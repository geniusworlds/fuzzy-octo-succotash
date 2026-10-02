#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include "macho_loader.hpp"
#include "darwin_runtime.hpp"
#include "arm64_core.hpp"
#include "ios_compositor.hpp"

// Utility to generate a synthetic valid 64-bit ARM64 Mach-O executable
std::vector<uint8_t> generateSyntheticArm64MachO() {
    std::vector<uint8_t> binary;

    MachHeader64 hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.magic = MH_MAGIC_64;
    hdr.cputype = CPU_TYPE_ARM64;
    hdr.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
    hdr.filetype = 0x2; // MH_EXECUTE
    hdr.ncmds = 3;
    hdr.flags = 0x00200085; // NOUNDEFS | DYLDLINK | TWOLEVEL | PIE

    // Commands:
    // 1. LC_SEGMENT_64 (__PAGEZERO)
    SegmentCommand64 segPageZero;
    std::memset(&segPageZero, 0, sizeof(segPageZero));
    segPageZero.cmd = LC_SEGMENT_64;
    segPageZero.cmdsize = sizeof(SegmentCommand64);
    std::strncpy(segPageZero.segname, "__PAGEZERO", 16);
    segPageZero.vmaddr = 0x0;
    segPageZero.vmsize = 0x100000000ULL; // 4GB pagezero
    segPageZero.fileoff = 0;
    segPageZero.filesize = 0;
    segPageZero.initprot = VM_PROT_NONE;
    segPageZero.maxprot = VM_PROT_NONE;

    // 2. LC_SEGMENT_64 (__TEXT)
    SegmentCommand64 segText;
    std::memset(&segText, 0, sizeof(segText));
    segText.cmd = LC_SEGMENT_64;
    segText.cmdsize = sizeof(SegmentCommand64);
    std::strncpy(segText.segname, "__TEXT", 16);
    segText.vmaddr = 0x100000000ULL;
    segText.vmsize = 0x4000; // 16KB
    segText.fileoff = 0;
    segText.filesize = 0x4000;
    segText.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
    segText.maxprot = VM_PROT_READ | VM_PROT_EXECUTE;

    // 3. LC_MAIN (entry point offset)
    EntryPointCommand epCmd;
    std::memset(&epCmd, 0, sizeof(epCmd));
    epCmd.cmd = LC_MAIN;
    epCmd.cmdsize = sizeof(EntryPointCommand);
    epCmd.entryoff = 0x1000; // offset within __TEXT
    epCmd.stacksize = 0x80000;

    hdr.sizeofcmds = segPageZero.cmdsize + segText.cmdsize + epCmd.cmdsize;

    // Assemble file
    binary.resize(0x4000, 0); // 16KB file size
    size_t cur = 0;

    std::memcpy(binary.data() + cur, &hdr, sizeof(hdr));
    cur += sizeof(hdr);

    std::memcpy(binary.data() + cur, &segPageZero, sizeof(segPageZero));
    cur += sizeof(segPageZero);

    std::memcpy(binary.data() + cur, &segText, sizeof(segText));
    cur += sizeof(segText);

    std::memcpy(binary.data() + cur, &epCmd, sizeof(epCmd));
    cur += sizeof(epCmd);

    // Place ARM64 instructions at entry point offset (0x1000)
    // 1. NOP: 0xD503201F
    // 2. MOV X16, #4 (write syscall) -> simplified: SVC #0x80 (0xD4001001)
    // 3. RET: 0xD65F03C0
    uint32_t* code = reinterpret_cast<uint32_t*>(binary.data() + 0x1000);
    code[0] = 0xD503201F; // NOP
    code[1] = 0xD4001001; // SVC #0x80 (Darwin Syscall)
    code[2] = 0xD65F03C0; // RET

    return binary;
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "   iOS ARM64 High-Level Emulation Engine on Android      " << std::endl;
    std::cout << "=========================================================\n" << std::endl;

    // Step 1: Initialize Darwin Runtime and Objective-C Engine
    auto runtime = std::make_shared<DarwinRuntime>();
    auto cpu = std::make_unique<Arm64Core>(runtime);
    auto compositor = std::make_unique<IOSCompositor>(390, 844);

    // Step 2: Generate and load synthetic 64-bit Mach-O binary
    std::vector<uint8_t> machOData = generateSyntheticArm64MachO();
    MachOLoader loader;
    if (!loader.loadFromBuffer(machOData.data(), machOData.size())) {
        std::cerr << "Failed to parse Mach-O binary!" << std::endl;
        return 1;
    }

    loader.printBinarySummary();

    // Step 3: Map Mach-O segments into Darwin virtual memory space
    for (const auto& seg : loader.getSegments()) {
        if (!seg.data.empty()) {
            runtime->writeMemory(seg.vmaddr, seg.data.data(), seg.data.size());
            std::cout << "[Loader] Mapped segment " << seg.name << " to 0x" 
                      << std::hex << seg.vmaddr << std::dec << std::endl;
        }
    }

    // Step 4: Setup CPU state & execute entry point
    cpu->setPC(loader.getEntryPoint());
    std::cout << "[CPU] Program Counter set to Entry Point: 0x" 
              << std::hex << loader.getEntryPoint() << std::dec << std::endl;
    std::cout << "[CPU] Stepping ARM64 execution pipeline..." << std::endl;
    cpu->executeInstructions(5);

    // Step 5: Simulate UIKit Application Lifecycle
    std::cout << "\n[UIKit] Initializing UIApplication and Root Window..." << std::endl;
    id app = runtime->objc_msgSend(nullptr, "sharedApplication");

    id window = runtime->createInstance("UIWindow");
    uint64_t w = 390, h = 844;
    runtime->objc_msgSend(window, "initWithFrame:", {0, 0, w, h});

    // Create a UILabel inside the window
    id label = runtime->createInstance("UILabel");
    uint64_t lw = 280, lh = 50;
    runtime->objc_msgSend(label, "initWithFrame:", {55, 180, lw, lh});
    runtime->objc_msgSend(label, "setText:", {reinterpret_cast<uint64_t>("Hello from iOS on Android")});
    runtime->objc_msgSend(window, "addSubview:", {reinterpret_cast<uint64_t>(label)});

    // Create a UIButton
    id button = runtime->createInstance("UIButton");
    uint64_t bw = 200, bh = 50;
    runtime->objc_msgSend(button, "initWithFrame:", {95, 260, bw, bh});
    runtime->objc_msgSend(button, "setTitle:forState:", {reinterpret_cast<uint64_t>("Launch iOS Feature"), 0});
    runtime->objc_msgSend(window, "addSubview:", {reinterpret_cast<uint64_t>(button)});

    // Make window key and visible
    runtime->objc_msgSend(window, "makeKeyAndVisible");

    // Step 6: Simulate Touch Event
    std::cout << "\n[Input] Simulating Touch Tap on iOS Screen..." << std::endl;
    runtime->triggerTouchEvent(150.0f, 280.0f, 0); // Tap on button

    // Step 7: Render Framebuffer
    std::cout << "\n[Compositor] Rendering iOS Framebuffer (390x844 ARGB)..." << std::endl;
    compositor->render(runtime);
    std::cout << "[Compositor] Successfully generated frame! (Buffer size: " 
              << compositor->getWidth() * compositor->getHeight() * 4 << " bytes)" << std::endl;

    // Step 8: Print all logged kernel and runtime output
    std::cout << "\n========== Execution Log Trace ==========" << std::endl;
    for (const auto& logLine : runtime->getConsoleLogs()) {
        std::cout << logLine << std::endl;
    }
    std::cout << "=========================================" << std::endl;

    return 0;
}
