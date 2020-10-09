#ifndef LAUNCHER_WINDOWS_H
#define LAUNCHER_WINDOWS_H

#include <imgui_vita2d/imgui_vita.h>
#include <imgui_vita2d/imgui_internal.h>

extern int selected_item;
extern int view_mode;

namespace Windows {
    inline void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };

    inline void SetNavFocusHere()
    {
        GImGui->NavId = GImGui->CurrentWindow->DC.LastItemId;
    }

    inline void ClearNavFocus()
    {
        GImGui->NavId = 0;
    }

    void HandleLauncherWindowInput();
    void LauncherWindow();
    void ShowGridViewWindow();
    void ShowListViewWindow();
    void ShowSettingsDialog();
    void GameScanWindow();
    void ShowTabBar();
    void HandleAddNewGame();

}

#endif
