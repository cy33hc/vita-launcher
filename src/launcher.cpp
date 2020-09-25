#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"

int selected_item = -1;
static SceCtrlData pad_prev;

namespace Windows {
    void HandleLauncherWindowInput()
    {
        SceCtrlData pad;
        sceCtrlPeekBufferNegative(0, &pad, 1);
        if ((pad_prev.buttons & SCE_CTRL_LTRIGGER) && !(pad.buttons & SCE_CTRL_LTRIGGER))
        {
            int prev_page = current_games->page_num;
            current_games->page_num = GAME::DecrementPage(current_games->page_num, 1);
            GAME::StartLoadImagesThread(prev_page, current_games->page_num);
        } else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) && !(pad.buttons & SCE_CTRL_RTRIGGER))
        {
            int prev_page = current_games->page_num;
            current_games->page_num = GAME::IncrementPage(current_games->page_num, 1);
            GAME::StartLoadImagesThread(prev_page, current_games->page_num);
        }
        pad_prev = pad;
    }

    void LauncherWindow() {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (ImGui::Begin(current_games->title, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            static int button_highlight = -1;
            int game_start_index = (current_games->page_num * 18) - 18;

            if (button_highlight > -1)
            {
                ImGui::Text("%s - %s", current_games->games[game_start_index+button_highlight].id, current_games->games[game_start_index+button_highlight].title);
            }
            else
            {
                ImGui::Text("No game selected");
            }

            ImVec2 pos = ImGui::GetCursorPos();
            for (int i = 0; i < 3; i++)
            {
                for (int j=0; j < 6; j++)
                {
                    ImGui::SetCursorPos(ImVec2(pos.x+(j*160),pos.y+(i*160)));
                    int button_id = (i*6)+j;
                    if (game_start_index+button_id < current_games->games.size())
                    {
                        ImGui::PushID(button_id);
                        Game *game = &current_games->games[game_start_index+button_id];
                        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(game->tex.id), ImVec2(138,128), ImVec2(0,0), ImVec2(1,1))) {
                            GAME::Launch(game->id);
                        }
                        if (button_highlight == button_id)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        if (ImGui::IsItemFocused())
                            button_highlight = button_id;
                        ImGui::PopID();

                        ImGui::SetCursorPosX(pos.x+(j*160));
                        ImGui::Text("%.15s", current_games->games[game_start_index+button_id].title);
                    }
                }
            }
            ImGui::SetCursorPos(ImVec2(pos.x, 524));
            ImGui::Text("Page#: %d", current_games->page_num);
        }

		ImGui::End();
        ImGui::PopStyleVar();
    }

    void GameScanWindow()
    {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (ImGui::Begin("Game Launcher", nullptr, ImGuiWindowFlags_NoDecoration)) {
            static float progress = 0.0f;
            if (current_games->games.size() > 0)
            {
                progress = (float)current_games->games.size() / (float)games_to_scan;
            }
            char buf[32];
            sprintf(buf, "%d/%d", current_games->games.size(), games_to_scan);
            ImGui::SetCursorPos(ImVec2(210, 230));
            ImGui::Text("Scanning games and creating cache in folder ux0:data/SMLA00001");
            ImGui::SetCursorPos(ImVec2(210, 260));
            ImGui::ProgressBar(progress, ImVec2(530, 0), buf);
            ImGui::SetCursorPos(ImVec2(210, 290));
            ImGui::Text("Adding %s to cache", game_scan_inprogress.id);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

}
