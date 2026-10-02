#include "darwin_runtime.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>

DarwinRuntime::DarwinRuntime() : nextFreeAddress(0x100000000ULL) {
    initializeRuntime();
}

DarwinRuntime::~DarwinRuntime() {}

void DarwinRuntime::log(const std::string& message) {
    std::string formatted = "[Darwin/iOS] " + message;
    consoleLogs.push_back(formatted);
    std::cout << formatted << std::endl;
}

void DarwinRuntime::initializeRuntime() {
    log("Booting Darwin / Mach XNU kernel user space compatibility layer...");

    // Register NSObject
    ObjcClass* nsObject = registerClass("NSObject", nullptr);
    registerMethod(nsObject, "init", [](id self, SEL sel, const auto& args) -> id {
        return self;
    });
    registerMethod(nsObject, "description", [](id self, SEL sel, const auto& args) -> id {
        return self;
    });

    // Register UIView
    ObjcClass* uiView = registerClass("UIView", nsObject);
    registerMethod(uiView, "initWithFrame:", [](id self, SEL sel, const auto& args) -> id {
        auto obj = reinterpret_cast<ObjcObject*>(self);
        if (args.size() >= 4) {
            obj->x = static_cast<float>(args[0]);
            obj->y = static_cast<float>(args[1]);
            obj->width = static_cast<float>(args[2]);
            obj->height = static_cast<float>(args[3]);
        }
        return self;
    });
    registerMethod(uiView, "addSubview:", [](id self, SEL sel, const auto& args) -> id {
        auto parent = reinterpret_cast<ObjcObject*>(self);
        if (!args.empty()) {
            auto child = reinterpret_cast<ObjcObject*>(reinterpret_cast<void*>(args[0]));
            if (child) {
                parent->subviews.push_back(std::shared_ptr<ObjcObject>(child, [](ObjcObject*){}));
            }
        }
        return nullptr;
    });

    // Register UIWindow
    ObjcClass* uiWindow = registerClass("UIWindow", uiView);
    registerMethod(uiWindow, "makeKeyAndVisible", [this](id self, SEL sel, const auto& args) -> id {
        auto obj = reinterpret_cast<ObjcObject*>(self);
        log("UIWindow: makeKeyAndVisible dispatched.");
        this->keyWindow = std::shared_ptr<ObjcObject>(obj, [](ObjcObject*){});
        return nullptr;
    });

    // Register UIViewController
    ObjcClass* uiViewController = registerClass("UIViewController", nsObject);
    registerMethod(uiViewController, "viewDidLoad", [this](id self, SEL sel, const auto& args) -> id {
        log("UIViewController: viewDidLoad invoked.");
        return nullptr;
    });

    // Register UILabel
    ObjcClass* uiLabel = registerClass("UILabel", uiView);
    registerMethod(uiLabel, "setText:", [](id self, SEL sel, const auto& args) -> id {
        auto obj = reinterpret_cast<ObjcObject*>(self);
        if (!args.empty()) {
            const char* str = reinterpret_cast<const char*>(args[0]);
            if (str) obj->stringValue = std::string(str);
        }
        return nullptr;
    });

    // Register UIButton
    ObjcClass* uiButton = registerClass("UIButton", uiView);
    registerMethod(uiButton, "setTitle:forState:", [](id self, SEL sel, const auto& args) -> id {
        auto obj = reinterpret_cast<ObjcObject*>(self);
        if (!args.empty()) {
            const char* str = reinterpret_cast<const char*>(args[0]);
            if (str) obj->stringValue = std::string(str);
        }
        return nullptr;
    });

    // Register UIApplication
    ObjcClass* uiApp = registerClass("UIApplication", nsObject);
    registerMethod(uiApp, "sharedApplication", [this](id self, SEL sel, const auto& args) -> id {
        static auto appInstance = reinterpret_cast<ObjcObject*>(this->createInstance("UIApplication"));
        return appInstance;
    });

    log("Darwin Core Foundation & UIKit class hierarchy successfully initialized.");
}

ObjcClass* DarwinRuntime::registerClass(const std::string& name, ObjcClass* superclass) {
    ObjcClass cls;
    cls.name = name;
    cls.superclass = superclass;
    registeredClasses[name] = cls;
    return &registeredClasses[name];
}

void DarwinRuntime::registerMethod(ObjcClass* cls, const std::string& sel, 
                                   std::function<id(id, SEL, const std::vector<uint64_t>&)> impl) {
    if (!cls) return;
    ObjcMethod m;
    m.name = sel;
    m.impl = impl;
    cls->methods[sel] = m;
}

id DarwinRuntime::createInstance(const std::string& className) {
    auto it = registeredClasses.find(className);
    if (it == registeredClasses.end()) {
        log("Error: Class '" + className + "' not registered in Objective-C runtime.");
        return nullptr;
    }
    ObjcObject* obj = new ObjcObject();
    obj->isa = &it->second;
    obj->x = 0; obj->y = 0; obj->width = 0; obj->height = 0;
    obj->hidden = false;
    obj->tag = 0;
    return reinterpret_cast<id>(obj);
}

id DarwinRuntime::objc_msgSend(id receiver, SEL selector, const std::vector<uint64_t>& args) {
    if (!receiver) return nullptr;
    ObjcObject* obj = reinterpret_cast<ObjcObject*>(receiver);
    if (!obj->isa) return nullptr;

    ObjcClass* curr = obj->isa;
    std::string selStr(selector);

    while (curr) {
        auto it = curr->methods.find(selStr);
        if (it != curr->methods.end()) {
            return it->second.impl(receiver, selector, args);
        }
        curr = curr->superclass;
    }

    log("Warning: Unrecognized selector -[" + obj->isa->name + " " + selStr + "]");
    return nullptr;
}

uint64_t DarwinRuntime::mach_vm_allocate(size_t size) {
    uint64_t addr = nextFreeAddress;
    // Align to 16KB page size (Apple ARM64 standard page size)
    size_t alignedSize = (size + 0x3FFF) & ~0x3FFF;
    virtualMemory[addr] = std::vector<uint8_t>(alignedSize, 0);
    nextFreeAddress += alignedSize;
    return addr;
}

void DarwinRuntime::writeMemory(uint64_t addr, const void* data, size_t size) {
    // Find matching block
    for (auto& pair : virtualMemory) {
        if (addr >= pair.first && (addr + size) <= (pair.first + pair.second.size())) {
            size_t offset = addr - pair.first;
            std::memcpy(pair.second.data() + offset, data, size);
            return;
        }
    }
    // Allocate if not found
    virtualMemory[addr] = std::vector<uint8_t>(size);
    std::memcpy(virtualMemory[addr].data(), data, size);
}

bool DarwinRuntime::readMemory(uint64_t addr, void* outData, size_t size) {
    for (const auto& pair : virtualMemory) {
        if (addr >= pair.first && (addr + size) <= (pair.first + pair.second.size())) {
            size_t offset = addr - pair.first;
            std::memcpy(outData, pair.second.data() + offset, size);
            return true;
        }
    }
    return false;
}

void DarwinRuntime::triggerTouchEvent(float x, float y, int phase) {
    std::ostringstream ss;
    ss << "Touch Event at (" << std::fixed << std::setprecision(1) << x << ", " << y 
       << ") Phase: " << (phase == 0 ? "Began" : (phase == 1 ? "Moved" : "Ended"));
    log(ss.str());

    // Hit-testing keyWindow and subviews
    if (keyWindow) {
        for (auto& subview : keyWindow->subviews) {
            if (x >= subview->x && x <= (subview->x + subview->width) &&
                y >= subview->y && y <= (subview->y + subview->height)) {
                log("Hit-test target: " + subview->isa->name + 
                    (subview->stringValue.empty() ? "" : " ('" + subview->stringValue + "')"));
            }
        }
    }
}

void DarwinRuntime::stepRunLoop() {
    // Process single iteration of CFRunLoop
}
