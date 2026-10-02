#ifndef IOS_COMPOSITOR_HPP
#define IOS_COMPOSITOR_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "darwin_runtime.hpp"

class IOSCompositor {
public:
    IOSCompositor(int width = 390, int height = 844);
    ~IOSCompositor();

    void resize(int width, int height);
    void render(std::shared_ptr<DarwinRuntime> runtime);

    const uint32_t* getFramebuffer() const { return framebuffer.data(); }
    int getWidth() const { return screenWidth; }
    int getHeight() const { return screenHeight; }

    void drawRect(int x, int y, int w, int h, uint32_t color);
    void drawText(int x, int y, const std::string& text, uint32_t color);
    void drawDynamicIsland();
    void drawHomeIndicator();

private:
    int screenWidth;
    int screenHeight;
    std::vector<uint32_t> framebuffer; // 32-bit ARGB/RGBA format
};

#endif // IOS_COMPOSITOR_HPP
