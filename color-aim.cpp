#define NOMINMAX
#include <windows.h>
#include <vector>
#include <cmath>
#include <atomic>
#include <iostream>
#include <algorithm>

struct Target { int x, y; };

constexpr int FOV_SIZE = 350;
constexpr BYTE TARGET_R = 0;
constexpr BYTE TARGET_G = 255;
constexpr BYTE TARGET_B = 255;
constexpr int COLOR_THRESHOLD = 40;
constexpr int AIM_OFFSET_Y = 11;
constexpr int MAX_STEP_X_BASE = 50;
constexpr int MAX_STEP_Y_BASE = 50;

std::atomic<bool> running(true);

template <typename T>
T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi ? hi : v);
}

inline bool isTargetColor(BYTE r, BYTE g, BYTE b) {
    int dr = r - TARGET_R;
    int dg = g - TARGET_G;
    int db = b - TARGET_B;
    return (dr * dr + dg * dg + db * db) <= COLOR_THRESHOLD * COLOR_THRESHOLD;
}

std::vector<Target> findAllTargets(HBITMAP hBitmap) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    std::vector<BYTE> pixels(bmp.bmWidth * bmp.bmHeight * 4);
    HDC hdc = CreateCompatibleDC(NULL);
    GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pixels.data(),
        reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
    DeleteDC(hdc);

    std::vector<Target> targets;
    for (int y = 0; y < bmp.bmHeight; ++y) {
        for (int x = 0; x < bmp.bmWidth; ++x) {
            int idx = (y * bmp.bmWidth + x) * 4;
            if (isTargetColor(pixels[idx + 2], pixels[idx + 1], pixels[idx])) {
                targets.push_back({ x, y });
            }
        }
    }
    return targets;
}

Target selectClosestTarget(const std::vector<Target>& targets, int fovSize) {
    if (targets.empty()) return { fovSize / 2, fovSize / 2 };

    int centerX = fovSize / 2;
    int centerY = fovSize / 2;

    Target closest = targets[0];
    double minDist = 1e9;

    for (const auto& t : targets) {
        double dx = t.x - centerX;
        double dy = t.y - centerY;
        double dist2 = dx * dx + dy * dy;
        if (dist2 < minDist) {
            minDist = dist2;
            closest = t;
        }
    }

    closest.y += AIM_OFFSET_Y;
    return closest;
}

void moveMouseToCenter(int dx, int dy) {
    if (dx == 0 && dy == 0) return;

    POINT p;
    GetCursorPos(&p);

    int MAX_STEP_X = MAX_STEP_X_BASE;
    int MAX_STEP_Y = MAX_STEP_Y_BASE;

    if (std::abs(dx) > 80) MAX_STEP_X = 150;
    if (std::abs(dy) > 80) MAX_STEP_Y = 100;

    if (std::abs(dx) < 20) dx = int(dx * 0.7);
    if (std::abs(dy) < 20) dy = int(dy * 0.7);

    dx = clamp(dx, -MAX_STEP_X, MAX_STEP_X);
    dy = clamp(dy, -MAX_STEP_Y, MAX_STEP_Y);

    SetCursorPos(p.x + dx, p.y + dy);
}

int main() {
    const char* pink = "\033[38;2;218;173;240m";
    const char* reset = "\033[0m";

    std::cout << pink << "**************************************************" << reset << std::endl;
    std::cout << pink << "*   Kawaii Krunker Color Tracker ~ UwU <3        *" << reset << std::endl;
    std::cout << pink << "**************************************************" << reset << std::endl;

    std::cout << pink << "                         _ __    __              " << reset << std::endl;
    std::cout << pink << "    ____  _  ___      __(_) /___/ /              " << reset << std::endl;
    std::cout << pink << "   / __ \\\\| |/_/ | /| / / / / __  /              " << reset << std::endl;
    std::cout << pink << "  / /_/ />  < | |/ |/ / / / /_/ /                " << reset << std::endl;
    std::cout << pink << " / .___/_/|_| |__/|__/_/_/\\\\__,_/                " << reset << std::endl;
    std::cout << pink << "/_/                                              " << reset << std::endl;

    std::cout << pink << "--------------------------------------------------" << reset << std::endl;
    std::cout << pink << "   Target Color: RGB(0, 255, 255)                 " << reset << std::endl;
    std::cout << pink << "   Game: Krunker                                 " << reset << std::endl;
    std::cout << pink << "   Hold RMB to activate tracking ^_^             " << reset << std::endl;
    std::cout << pink << "   Made with love ~ pxwild  >w<                  " << reset << std::endl;
    std::cout << pink << "   Stay kawaii, stay smooth, have fun! <3        " << reset << std::endl;
    std::cout << pink << "--------------------------------------------------" << reset << std::endl << std::endl;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    const int fov = (int)std::min(FOV_SIZE, std::min(vw, vh));

    const int centerX = vx + vw / 2;
    const int centerY = vy + vh / 2;

    int srcLeft = clamp(centerX - fov / 2, vx, vx + vw - fov);
    int srcTop = clamp(centerY - fov / 2, vy, vy + vh - fov);

    while (running) {
        if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
            Sleep(1);
            continue;
        }

        srcLeft = clamp(centerX - fov / 2, vx, vx + vw - fov);
        srcTop = clamp(centerY - fov / 2, vy, vy + vh - fov);

        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, fov, fov);
        HGDIOBJ oldObj = SelectObject(hdcMem, hBitmap);

        BitBlt(hdcMem, 0, 0, fov, fov, hdcScreen, srcLeft, srcTop, SRCCOPY);

        std::vector<Target> targets = findAllTargets(hBitmap);
        Target t = selectClosestTarget(targets, fov);

        int dx = t.x - fov / 2;
        int dy = t.y - fov / 2;
        moveMouseToCenter(dx, dy);

        SelectObject(hdcMem, oldObj);
        DeleteObject(hBitmap);

        Sleep(1);
    }

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return 0;
}
