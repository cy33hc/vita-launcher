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
            int prev_page = page_num;
            page_num = GAME::DecrementPage(page_num, 1);
            GAME::StartLoadImagesThread(prev_page, page_num);
        } else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) && !(pad.buttons & SCE_CTRL_RTRIGGER))
        {
            int prev_page = page_num;
            page_num = GAME::IncrementPage(page_num, 1);
            GAME::StartLoadImagesThread(prev_page, page_num);
        }
        pad_prev = pad;
    }

    void LauncherWindow(bool *focus, bool *first_item) {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

   		if (ImGui::Begin("Game Launcher", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
			static int button_highlight = -1;
            int game_start_index = (page_num * 18) - 18;

			if (button_highlight > -1)
			{
				ImGui::Text("%s - %s", games[game_start_index+button_highlight].id, games[game_start_index+button_highlight].title);
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
                    if (game_start_index+button_id < games.size())
                    {
                        ImGui::PushID(button_id);
                        Game *game = &games[game_start_index+button_id];
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
                        ImGui::Text("%.15s", games[game_start_index+button_id].title);
                    }
                }
            }
            ImGui::SetCursorPos(ImVec2(pos.x, 524));
            ImGui::Text("Page#: %d", page_num);
        }

		ImGui::End();
        ImGui::PopStyleVar();
    }
}
