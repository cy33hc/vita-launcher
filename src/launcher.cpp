#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"

int selected_item = -1;

namespace Windows {
    void LauncherWindow(bool *focus, bool *first_item) {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

   		if (ImGui::Begin("Game Launcher", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
			static int button_highlight = -1;
            int game_start_index = (page_num * 18) - 18;

			if (button_highlight > -1)
			{
				ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), "%s - %s", games[game_start_index+button_highlight].id, games[game_start_index+button_highlight].title);
			}
			else
			{
				ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), "No game selected");
			}

            ImVec2 pos = ImGui::GetCursorPos();
            for (int i = 0; i < 3; i++)
            {
                for (int j=0; j < 6; j++)
                {
                    ImGui::SetCursorPos(ImVec2(pos.x+(j*160),pos.y+(i*158)));
                    int button_id = (i*6)+j;
                    ImGui::PushID(button_id);
                    Game *game = &games[game_start_index+button_id];
                    if (games->tex.id < 0)
                    {
                        GAME::LoadGameImage(game);
                    }
                    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(game->tex.id), ImVec2(138,130), ImVec2(0,0), ImVec2(1,1))) {
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
                    ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), "%.15s", games[game_start_index+button_id].title);

                }

            }
            ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), "Page#: %d", page_num);
        }

		ImGui::End();
        ImGui::PopStyleVar();
    }
}
