#pragma once
#include <raylib.h>

namespace ImGui
{
    // Initialization
    bool InitRaylib();
    void NewFrame();
    void RenderImGui();
    void ShutdownImGui();

    // Keyboard/Mouse helpers
    bool IsKeyPressed(int key);
    bool IsKeyDown(int key);
    bool IsKeyReleased(int key);
    int GetKeyPressedChar();
    int GetMouseX();
    int GetMouseY();
    bool IsMouseDown(int button);
    float GetMouseWheelMove();
    void ShowMouseCursor(bool show);

    // Process events (optional)
    bool ProcessEvent();
}