#include "imgui.h"
#include "imgui_impl_raylib.h"
#include <raylib.h>

static double g_Time = 0.0;

static const char* ImGui_ImplRaylib_GetClipboardText(void*)
{
    return GetClipboardText();
}

static void ImGui_ImplRaylib_SetClipboardText(void*, const char* text)
{
    SetClipboardText(text);
}

bool ImGui::InitRaylib()
{
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = "imgui_impl_raylib";
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    io.SetClipboardTextFn = ImGui_ImplRaylib_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplRaylib_GetClipboardText;
    io.ClipboardUserData = NULL;

    return true;
}

void ImGui::ShutdownImGui()
{
    g_Time = 0.0;
}

static void ImGui_ImplRaylib_UpdateMousePosAndButtons()
{
    ImGuiIO& io = ImGui::GetIO();

    if (!IsWindowMinimized()) {
        io.MousePos = ImVec2((float)GetMouseX(), (float)GetMouseY());
    }

    io.MouseDown[0] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    io.MouseDown[1] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    io.MouseDown[2] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
}

void ImGui::NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)GetScreenWidth(), (float)GetScreenHeight());

    double current_time = GetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
    g_Time = current_time;

    // Mouse wheel
    if (GetMouseWheelMove() > 0)
        io.MouseWheel += 1;
    else if (GetMouseWheelMove() < 0)
        io.MouseWheel -= 1;

    ImGui_ImplRaylib_UpdateMousePosAndButtons();
}

bool ImGui::ProcessEvent()
{
    // Simple event processing - character input
    ImGuiIO& io = ImGui::GetIO();
    int key = GetKeyPressed();
    if (key != -1) {
        // Add character to ImGui input buffer
        char c = (char)key;
        io.AddInputCharacter(c);
    }
    return true;
}

bool ImGui::IsKeyDown(int key) { return ::IsKeyDown(key); }
bool ImGui::IsKeyPressed(int key) { return ::IsKeyPressed(key); }
bool ImGui::IsKeyReleased(int key) { return ::IsKeyReleased(key); }
int ImGui::GetKeyPressedChar() { return GetKeyPressed(); }
int ImGui::GetMouseX() { return GetMouseX(); }
int ImGui::GetMouseY() { return GetMouseY(); }
bool ImGui::IsMouseDown(int button) { return IsMouseButtonDown(button); }
float ImGui::GetMouseWheelMove() { return (float)::GetMouseWheelMove(); }
void ImGui::ShowMouseCursor(bool show) { if (show) ShowCursor(); else HideCursor(); }

// Simple ImGui renderer using Raylib primitives (no rlgl dependency)
void ImGui::RenderImGui()
{
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (draw_data->CmdListsCount == 0) return;

    int w = (int)ImGui::GetIO().DisplaySize.x;
    int h = (int)ImGui::GetIO().DisplaySize.y;
    ImVec2 display_pos = ImGui::GetIO().DisplayPos;

    // Disable depth testing for 2D rendering
    rlDisableDepthMask();

    const float x_off = display_pos.x;
    const float y_off = display_pos.y;

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &(cmd_list->CmdBuffer.Data)[cmd_i];

            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                // Clip rect - use full screen since we can't set scissor easily
                int clip_l = (int)(pcmd->ClipRect.x - x_off);
                int clip_t = (int)(pcmd->ClipRect.y - y_off);
                int clip_r = (int)(pcmd->ClipRect.z - x_off);
                int clip_b = (int)(pcmd->ClipRect.w - y_off);

                // Get texture ID from user pointer (older ImGui uses TextureId as void*)
                #ifdef IMGUI_HAS_DRAW_LIST_NO_TEXTURE
                bool has_texture = (pcmd->TextureId != nullptr);
                #else
                void* tex_ptr = pcmd->TextureId;
                bool has_texture = (tex_ptr != nullptr);
                #endif

                if (!has_texture) {
                    // No texture - use solid color drawing with rlBegin/rlEnd
                    for (unsigned int idx = 0; idx < pcmd->ElemCount / 3; idx++) {
                        unsigned int v0 = pcmd->IdxBuffer[idx * 3];
                        unsigned int v1 = pcmd->IdxBuffer[idx * 3 + 1];
                        unsigned int v2 = pcmd->IdxBuffer[idx * 3 + 2];

                        const ImDrawVert& vt0 = vtx_buffer[v0];
                        const ImDrawVert& vt1 = vtx_buffer[v1];
                        const ImDrawVert& vt2 = vtx_buffer[v2];

                        // Extract color from vertex (BGRA)
                        unsigned char r0 = (unsigned char)((vt0.col >> 0) & 0xFF);
                        unsigned char g0 = (unsigned char)((vt0.col >> 8) & 0xFF);
                        unsigned char b0 = (unsigned char)((vt0.col >> 16) & 0xFF);
                        unsigned char a0 = (unsigned char)((vt0.col >> 24) & 0xFF);

                        unsigned char r1 = (unsigned char)((vt1.col >> 0) & 0xFF);
                        unsigned char g1 = (unsigned char)((vt1.col >> 8) & 0xFF);
                        unsigned char b1 = (unsigned char)((vt1.col >> 16) & 0xFF);
                        unsigned char a1 = (unsigned char)((vt1.col >> 24) & 0xFF);

                        unsigned char r2 = (unsigned char)((vt2.col >> 0) & 0xFF);
                        unsigned char g2 = (unsigned char)((vt2.col >> 8) & 0xFF);
                        unsigned char b2 = (unsigned char)((vt2.col >> 16) & 0xFF);
                        unsigned char a2 = (unsigned char)((vt2.col >> 24) & 0xFF);

                        rlBegin(RL_TRIANGLES);
                        rlColor4ub(r0, g0, b0, a0);
                        rlVertex2v(rlVector2(vt0.pos.x + x_off, vt0.pos.y + y_off));
                        rlColor4ub(r1, g1, b1, a1);
                        rlVertex2v(rlVector2(vt1.pos.x + x_off, vt1.pos.y + y_off));
                        rlColor4ub(r2, g2, b2, a2);
                        rlVertex2v(rlVector2(vt2.pos.x + x_off, vt2.pos.y + y_off));
                        rlEnd();
                    }
                } else {
                    // With texture - skip for now (simplified renderer)
                    // This handles text rendering which uses textures
                    // For a full implementation we'd need rlgl
                }
            }
        }
    }

    rlEnableDepthMask();
}