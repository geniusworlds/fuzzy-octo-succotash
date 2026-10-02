#include <jni.h>
#include <string>
#include <memory>
#include <android/bitmap.h>
#include "../../../../ios_engine/macho_loader.hpp"
#include "../../../../ios_engine/darwin_runtime.hpp"
#include "../../../../ios_engine/arm64_core.hpp"
#include "../../../../ios_engine/ios_compositor.hpp"

static std::shared_ptr<DarwinRuntime> g_runtime;
static std::unique_ptr<Arm64Core> g_cpu;
static std::unique_ptr<IOSCompositor> g_compositor;

extern "C" {

JNIEXPORT void JNICALL
Java_com_iosvm_emulator_MainActivity_nativeInit(JNIEnv* env, jobject thiz, jint width, jint height) {
    g_runtime = std::make_shared<DarwinRuntime>();
    g_cpu = std::make_unique<Arm64Core>(g_runtime);
    g_compositor = std::make_unique<IOSCompositor>(width, height);
}

JNIEXPORT jboolean JNICALL
Java_com_iosvm_emulator_MainActivity_nativeLoadApp(JNIEnv* env, jobject thiz, jstring pathStr) {
    if (!g_runtime) return JNI_FALSE;

    const char* nativePath = env->GetStringUTFChars(pathStr, nullptr);
    MachOLoader loader;
    bool success = loader.loadFromFile(nativePath);
    env->ReleaseStringUTFChars(pathStr, nativePath);

    if (!success) {
        g_runtime->log("Failed to parse 64-bit Mach-O executable.");
        return JNI_FALSE;
    }

    // Map segments into virtual address space
    for (const auto& seg : loader.getSegments()) {
        if (!seg.data.empty()) {
            g_runtime->writeMemory(seg.vmaddr, seg.data.data(), seg.data.size());
        }
    }

    g_cpu->setPC(loader.getEntryPoint());
    g_runtime->log("Mach-O binary loaded and mapped. Entry point set.");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_iosvm_emulator_MainActivity_nativeRenderFrame(JNIEnv* env, jobject thiz, jobject bitmap) {
    if (!g_compositor || !g_runtime) return;

    AndroidBitmapInfo info;
    void* pixels = nullptr;

    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) return;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return;

    // Render iOS frame
    g_compositor->render(g_runtime);

    const uint32_t* src = g_compositor->getFramebuffer();
    size_t byteCount = info.width * info.height * 4;
    std::memcpy(pixels, src, std::min(byteCount, (size_t)(g_compositor->getWidth() * g_compositor->getHeight() * 4)));

    AndroidBitmap_unlockPixels(env, bitmap);
}

JNIEXPORT void JNICALL
Java_com_iosvm_emulator_MainActivity_nativeTouchEvent(JNIEnv* env, jobject thiz, jfloat x, jfloat y, jint action) {
    if (g_runtime) {
        g_runtime->triggerTouchEvent(x, y, action);
    }
}

JNIEXPORT jstring JNICALL
Java_com_iosvm_emulator_MainActivity_nativeGetLogs(JNIEnv* env, jobject thiz) {
    std::string allLogs;
    if (g_runtime) {
        for (const auto& line : g_runtime->getConsoleLogs()) {
            allLogs += line + "\n";
        }
    }
    return env->NewStringUTF(allLogs.c_str());
}

} // extern "C"
