# iOS ARM64 High-Level Emulation (HLE) & Runtime for Android

## 1. Architectural Strategy
To achieve iOS execution on an Android device without the impassable roadblocks of Apple's closed-source hardware virtualization (missing AIC, SEP, PMGR, and Metal drivers), this project implements a **High-Level Emulation (HLE) Compatibility Layer** (following the architectural paradigm of Wine, Darling, and touchHLE).

Rather than attempting to boot a full XNU kernel inside a virtual machine (which fails due to hardware encryption and lack of EL2/KVM on Android), this architecture:
1. **Parses 64-bit ARM64 Mach-O Binaries:** Reads headers, load commands (`LC_SEGMENT_64`, `LC_MAIN`), and maps executable segments (`__TEXT`, `__DATA`).
2. **Executes ARM64 Code:** Runs instructions directly on the host ARM64 CPU or through an instruction stepper with Darwin syscall interception (`SVC 0x80`).
3. **Objective-C / Darwin Runtime:** Emulates `objc_msgSend`, class registration, and memory management (`mach_vm_allocate`).
4. **UIKit & Windowing Compositor:** Implements core UI components (`UIWindow`, `UIView`, `UILabel`, `UIButton`), handles touch events, and renders directly to an Android `Bitmap` framebuffer at 60 FPS.

---

## 2. Directory Structure

```
├── ios_engine/
│   ├── macho_loader.hpp / .cpp    # 64-bit Mach-O binary parser
│   ├── darwin_runtime.hpp / .cpp  # Darwin Mach & Objective-C runtime
│   ├── arm64_core.hpp / .cpp      # ARM64 CPU state & syscall traps
│   ├── ios_compositor.hpp / .cpp  # Framebuffer renderer (390x844 ARGB)
│   └── test_runner.cpp            # Standalone verification test harness
│
└── android_app/
    ├── app/
    │   ├── build.gradle           # NDK & CMake build definitions
    │   └── src/main/
    │       ├── AndroidManifest.xml
    │       ├── cpp/
    │       │   ├── CMakeLists.txt # Native build script
    │       │   └── native_bridge.cpp # JNI bindings
    │       ├── java/com/iosvm/emulator/
    │       │   └── MainActivity.kt   # UI & 60 FPS render loop
    │       └── res/layout/
    │           └── activity_main.xml # Layout with iOS viewport
    ├── build.gradle
    └── settings.gradle
```

---

## 3. How to Build the Android APK

Requirements:
- Android Studio Hedgehog / Iguana (or newer)
- Android NDK (r25c or newer)
- CMake 3.22.1+

Steps:
1. Open the `android_app` folder in Android Studio.
2. Let Gradle sync project dependencies and NDK toolchain.
3. Connect an ARM64 Android device (or run an arm64-v8a emulator).
4. Run `Build -> Make Project` or execute:
   ```bash
   ./gradlew assembleDebug
   ```
5. Install the resulting APK:
   ```bash
   adb install -r app/build/outputs/apk/debug/app-debug.apk
   ```
