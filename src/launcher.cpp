#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"

Game *selected_game;
static SceCtrlData pad_prev;
static int button_highlight = -1;
static bool paused = false;

namespace Windows {
    void HandleLauncherWindowInput()
    {
        SceCtrlData pad;
        sceCtrlPeekBufferNegative(0, &pad, 1);

        if ((pad_prev.buttons & SCE_CTRL_SQUARE) && !(pad.buttons & SCE_CTRL_SQUARE) && !paused)
        {
            if (selected_game != nullptr && current_category->id != FAVORITES)
            {
                if (!selected_game->favorite)
                {
                    Game game = *selected_game;
                    game.tex = no_icon;
                    game_categories[FAVORITES].games.push_back(game);
                    GAME::SortGames(&game_categories[FAVORITES]);
                    GAME::SetMaxPage(&game_categories[FAVORITES]);
                    selected_game->favorite = true;
                    GAME::SaveFavorites();
                }
                else {
                    selected_game->favorite = false;
                    int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game->id);
                    if (index != -1)
                        GAME::SaveFavorites();
                }
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_TRIANGLE) && !(pad.buttons & SCE_CTRL_TRIANGLE) && !paused)
        {
        }

        if ((pad_prev.buttons & SCE_CTRL_CIRCLE) && !(pad.buttons & SCE_CTRL_CIRCLE) && !paused)
        {
            GameCategory *previous_category = current_category;
            current_category = &game_categories[(current_category->id + 1) % 4 ];
            button_highlight = -1;

            GAME::DeleteGamesImages(previous_category);
            GAME::StartLoadImagesThread(current_category->page_num, current_category->page_num);
        }

        if ((pad_prev.buttons & SCE_CTRL_LTRIGGER) &&
            !(pad.buttons & SCE_CTRL_LTRIGGER) &&
            current_category->max_page > 1 && !paused)
        {
            int prev_page = current_category->page_num;
            current_category->page_num = GAME::DecrementPage(current_category->page_num, 1);
            GAME::StartLoadImagesThread(prev_page, current_category->page_num);
            button_highlight = -1;
            selected_game = nullptr;
        } else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) &&
                   !(pad.buttons & SCE_CTRL_RTRIGGER) &&
                   current_category->max_page > 1 && !paused)
        {
            int prev_page = current_category->page_num;
            current_category->page_num = GAME::IncrementPage(current_category->page_num, 1);
            GAME::StartLoadImagesThread(prev_page, current_category->page_num);
            button_highlight = -1;
            selected_game = nullptr;
        }
        pad_prev = pad;
    }

    void LauncherWindow() {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (ImGui::Begin(current_category->title, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            int game_start_index = (current_category->page_num * 18) - 18;

            if (button_highlight > -1)
            {
                ImGui::Text("%s - %s", current_category->games[game_start_index+button_highlight].id, current_category->games[game_start_index+button_highlight].title);
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
                    if (game_start_index+button_id < current_category->games.size())
                    {
                        ImGui::PushID(button_id);
                        Game *game = &current_category->games[game_start_index+button_id];
                        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(game->tex.id), ImVec2(138,128), ImVec2(0,0), ImVec2(1,1))) {
                            GAME::Launch(game->id);
                        }
                        if (button_highlight == button_id)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        if (ImGui::IsItemFocused())
                        {
                            button_highlight = button_id;
                            selected_game = game;
                        }
                        ImGui::PopID();

                        ImGui::SetCursorPosX(pos.x+(j*160));
                        if (game->favorite)
                        {
                            ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(pos.x+(j*160)+14);
                            ImGui::Text("%.14s", game->title);
                        }
                        else
                        {
                            ImGui::SetCursorPosX(pos.x+(j*160));
                            ImGui::Text("%.15s", game->title);
                        }
                    }
                }
            }
            ImGui::SetCursorPos(ImVec2(pos.x, 524));
            ImGui::Text("Page#: %d/%d", current_category->page_num, current_category->max_page);

            if (io.NavInputs[ImGuiNavInput_Input] == 1.0f)
            {
                paused = true;
                ImGui::OpenPopup("Scan Games");
            }

            ImGui::SetNextWindowPos(ImVec2(400, 250));
            if (ImGui::BeginPopupModal("Scan Games"))
            {
                ImGui::Text("Refresh games cache?");
                ImGui::Separator();
                if (ImGui::Button("OK"))
                {
                    GAME::RefreshGames();
                    paused = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    paused = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
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
            if (current_category->games.size() > 0)
            {
                progress = (float)current_category->games.size() / (float)games_to_scan;
            }
            char buf[32];
            sprintf(buf, "%d/%d", current_category->games.size(), games_to_scan);
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
