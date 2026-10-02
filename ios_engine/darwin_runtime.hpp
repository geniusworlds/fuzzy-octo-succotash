#ifndef DARWIN_RUNTIME_HPP
#define DARWIN_RUNTIME_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>

// Objective-C Type definitions
typedef void* id;
typedef const char* SEL;

struct ObjcClass;
struct ObjcMethod {
    std::string name;
    std::function<id(id, SEL, const std::vector<uint64_t>&)> impl;
};

struct ObjcClass {
    std::string name;
    ObjcClass* superclass;
    std::map<std::string, ObjcMethod> methods;
};

struct ObjcObject {
    ObjcClass* isa;
    std::map<std::string, uint64_t> ivars;
    std::string stringValue; // For NSString/UILabel
    uint32_t tag;
    float x, y, width, height; // Geometry for UIViews
    bool hidden;
    std::vector<std::shared_ptr<ObjcObject>> subviews;
};

// UIKit Application Delegate protocol representation
class DarwinRuntime {
public:
    DarwinRuntime();
    ~DarwinRuntime();

    void initializeRuntime();

    // Objective-C Runtime Engine
    ObjcClass* registerClass(const std::string& name, ObjcClass* superclass = nullptr);
    void registerMethod(ObjcClass* cls, const std::string& sel, 
                        std::function<id(id, SEL, const std::vector<uint64_t>&)> impl);
    
    id objc_msgSend(id receiver, SEL selector, const std::vector<uint64_t>& args = {});
    id createInstance(const std::string& className);

    // Memory management & Mach task
    uint64_t mach_vm_allocate(size_t size);
    void writeMemory(uint64_t addr, const void* data, size_t size);
    bool readMemory(uint64_t addr, void* outData, size_t size);

    // UIKit Simulation
    std::shared_ptr<ObjcObject> getRootWindow() const { return keyWindow; }
    void triggerTouchEvent(float x, float y, int phase); // 0=begin, 1=move, 2=end
    void stepRunLoop();

    // Logging
    const std::vector<std::string>& getConsoleLogs() const { return consoleLogs; }
    void log(const std::string& message);

private:
    std::map<std::string, ObjcClass> registeredClasses;
    std::map<uint64_t, std::vector<uint8_t>> virtualMemory;
    uint64_t nextFreeAddress;

    std::shared_ptr<ObjcObject> keyWindow;
    std::shared_ptr<ObjcObject> rootViewController;
    std::vector<std::string> consoleLogs;
};

#endif // DARWIN_RUNTIME_HPP
