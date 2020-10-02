#ifndef LAUNCHER_WINDOWS_H
#define LAUNCHER_WINDOWS_H

#include <imgui_vita2d/imgui_vita.h>

extern int selected_item;
extern int view_mode;

namespace Windows {
    inline void SetupWindow(void) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    };

    void HandleLauncherWindowInput();
    void LauncherWindow();
    void ShowGridViewWindow();
    void ShowListViewWindow();
    void ShowSettingsDialog();
    void HandleLaunchError();
    void GameScanWindow();
}

#endif
