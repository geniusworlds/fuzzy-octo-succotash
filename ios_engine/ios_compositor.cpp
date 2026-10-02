#include "ios_compositor.hpp"
#include <algorithm>
#include <cmath>

// ARGB colors
constexpr uint32_t COLOR_BG = 0xFFF2F2F7;         // iOS Light Background
constexpr uint32_t COLOR_DARK_BG = 0xFF1C1C1E;    // iOS Dark Background
constexpr uint32_t COLOR_BLACK = 0xFF000000;
constexpr uint32_t COLOR_WHITE = 0xFFFFFFFF;
constexpr uint32_t COLOR_BLUE = 0xFF007AFF;       // iOS System Blue
constexpr uint32_t COLOR_GRAY = 0xFF8E8E93;       // iOS System Gray
constexpr uint32_t COLOR_ACCENT = 0xFF34C759;     // iOS Green

IOSCompositor::IOSCompositor(int width, int height) 
    : screenWidth(width), screenHeight(height), framebuffer(width * height, COLOR_BG) {
}

IOSCompositor::~IOSCompositor() {}

void IOSCompositor::resize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    framebuffer.resize(width * height, COLOR_BG);
}

void IOSCompositor::drawRect(int x, int y, int w, int h, uint32_t color) {
    int startX = std::max(0, x);
    int startY = std::max(0, y);
    int endX = std::min(screenWidth, x + w);
    int endY = std::min(screenHeight, y + h);

    for (int cy = startY; cy < endY; ++cy) {
        for (int cx = startX; cx < endX; ++cx) {
            framebuffer[cy * screenWidth + cx] = color;
        }
    }
}

void IOSCompositor::drawDynamicIsland() {
    int islandW = 120;
    int islandH = 34;
    int islandX = (screenWidth - islandW) / 2;
    int islandY = 12;

    // Draw rounded pill for Dynamic Island
    drawRect(islandX, islandY, islandW, islandH, COLOR_BLACK);
}

void IOSCompositor::drawHomeIndicator() {
    int barW = 140;
    int barH = 5;
    int barX = (screenWidth - barW) / 2;
    int barY = screenHeight - 14;

    drawRect(barX, barY, barW, barH, COLOR_BLACK);
}

void IOSCompositor::drawText(int x, int y, const std::string& text, uint32_t color) {
    // Basic 5x7 bitmap font rendering for simulation
    static const uint8_t font5x7[128][7] = {
        // Simple printable characters for status bar / labels
    };
    // Placeholder block representing text layout bounds
    int estWidth = static_cast<int>(text.length()) * 8;
    int estHeight = 14;
    drawRect(x, y, estWidth, estHeight, color);
}

void IOSCompositor::render(std::shared_ptr<DarwinRuntime> runtime) {
    // 1. Clear background
    std::fill(framebuffer.begin(), framebuffer.end(), COLOR_BG);

    // 2. Render Status Bar
    drawRect(0, 0, screenWidth, 48, 0x10000000); // subtle status bar gradient
    drawDynamicIsland();

    // 3. Render active window and views from Objective-C hierarchy
    if (runtime) {
        auto keyWin = runtime->getRootWindow();
        if (keyWin) {
            for (const auto& view : keyWin->subviews) {
                if (!view->hidden) {
                    uint32_t viewColor = COLOR_BLUE;
                    if (view->isa && view->isa->name == "UILabel") {
                        viewColor = COLOR_DARK_BG;
                    } else if (view->isa && view->isa->name == "UIButton") {
                        viewColor = COLOR_ACCENT;
                    }
                    drawRect(static_cast<int>(view->x), static_cast<int>(view->y),
                             static_cast<int>(view->width), static_cast<int>(view->height),
                             viewColor);
                }
            }
        } else {
            // Draw default SpringBoard home grid simulation
            int iconSize = 60;
            int padding = 28;
            int startX = 32;
            int startY = 80;

            for (int row = 0; row < 5; ++row) {
                for (int col = 0; col < 4; ++col) {
                    int ix = startX + col * (iconSize + padding);
                    int iy = startY + row * (iconSize + padding + 16);
                    uint32_t iconColor = (row == 0 && col == 0) ? COLOR_BLUE : 
                                         (row == 0 && col == 1) ? COLOR_ACCENT : COLOR_GRAY;
                    drawRect(ix, iy, iconSize, iconSize, iconColor);
                }
            }

            // Draw Dock container
            int dockH = 90;
            int dockY = screenHeight - dockH - 30;
            drawRect(18, dockY, screenWidth - 36, dockH, 0x30FFFFFF);

            // Dock icons
            for (int i = 0; i < 4; ++i) {
                int dx = 36 + i * (iconSize + 22);
                int dy = dockY + 15;
                drawRect(dx, dy, iconSize, iconSize, COLOR_BLUE);
            }
        }
    }

    // 4. Render Home Indicator bar
    drawHomeIndicator();
}
