#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <algorithm> 
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"
#include "config.h"
extern "C" {
	#include "inifile.h"
}

Game *selected_game;
static SceCtrlData pad_prev;
bool paused = false;
bool launch_success = true;
bool handle_add_game = false;
bool game_added = false;
int view_mode;
static std::vector<std::string> games_on_filesystem;

namespace Windows {
    void HandleLauncherWindowInput()
    {
        SceCtrlData pad;
        sceCtrlPeekBufferNegative(0, &pad, 1);

        if ((pad_prev.buttons & SCE_CTRL_SQUARE) && !(pad.buttons & SCE_CTRL_SQUARE) && !paused)
        {
            if (selected_game != nullptr)
            {
                if (current_category->id != FAVORITES)
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
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        if (index != -1)
                            GAME::SaveFavorites();
                    }
                }
                else
                {
                    Game* game = nullptr;
                    for (int i=0; i<3; i++)
                    {
                        game = GAME::FindGame(&game_categories[i], selected_game->id);
                        if (game != nullptr)
                        {
                            break;
                        }
                    }
                    if (game != nullptr)
                    {
                        game->favorite = false;
                        int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game->id);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        if (index != -1)
                            GAME::SaveFavorites();
                        selected_game = nullptr;
                    }
                }
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_CIRCLE) && !(pad.buttons & SCE_CTRL_CIRCLE) && !paused)
        {
            GameCategory *previous_category = current_category;
            current_category = &game_categories[(current_category->id + 1) % 4 ];
            while (current_category->games.size() == 0)
            {
                current_category = &game_categories[(current_category->id + 1) % 4 ];
            }
            view_mode = current_category->view_mode;
            selected_game = nullptr;

            GAME::DeleteGamesImages(previous_category);
            GAME::StartLoadImagesThread(current_category->id, current_category->page_num, current_category->page_num);
        }

        if ((pad_prev.buttons & SCE_CTRL_LTRIGGER) &&
            !(pad.buttons & SCE_CTRL_LTRIGGER) &&
            current_category->view_mode == VIEW_MODE_GRID &&
            current_category->max_page > 1 && !paused)
        {
            int prev_page = current_category->page_num;
            current_category->page_num = GAME::DecrementPage(current_category->page_num, 1);
            GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->page_num);
            selected_game = nullptr;
        } else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) &&
                   !(pad.buttons & SCE_CTRL_RTRIGGER) &&
                   current_category->view_mode == VIEW_MODE_GRID &&
                   current_category->max_page > 1 && !paused)
        {
            int prev_page = current_category->page_num;
            current_category->page_num = GAME::IncrementPage(current_category->page_num, 1);
            GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->page_num);
            selected_game = nullptr;
        }
        pad_prev = pad;
    }

    void LauncherWindow() {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (current_category->view_mode == VIEW_MODE_GRID)
        {
            ShowGridViewWindow();
        }
        else if (current_category->view_mode == VIEW_MODE_LIST)
        {
            ShowListViewWindow();
        }

        if (!launch_success)
        {
            HandleLaunchError();
        }

        if (handle_add_game)
        {
            HandleAddNewGame();
        }

        ShowSettingsDialog();

        ImGui::PopStyleVar();
    }

    void ShowGridViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.05f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(50000);

        if (ImGui::Begin(current_category->title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
        {
            int game_start_index = (current_category->page_num * 18) - 18;

            if (selected_game != nullptr)
            {
                ImGui::Text("%s - %s", selected_game->id, selected_game->title);
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
                            launch_success = GAME::Launch(game->id);
                        }
                        if (ImGui::IsItemFocused())
                        {
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
        }
        ImGui::End();
    }

    void ShowListViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.005f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(1000);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        static int position = -1;
        static float scroll_direction = 0.0f;
        if (io.NavInputs[ImGuiNavInput_DpadRight] == 1.0f)
        {
            if (selected_game != nullptr)
            {
                position = GAME::FindGamePosition(current_category, selected_game->id);
                position += 5;
                if (position > current_category->games.size()-1)
                {
                    position = current_category->games.size()-1;
                }
                scroll_direction = 1.0f;
            }
        } else if (io.NavInputs[ImGuiNavInput_DpadLeft] == 1.0f)
        {
            if (selected_game != nullptr)
            {
                position = GAME::FindGamePosition(current_category, selected_game->id);
                position -= 5;
                if (position < 0)
                {
                    position = 0;
                }
                scroll_direction = 0.0f;
            }
        }

        if (ImGui::Begin(current_category->title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysUseWindowPadding))
        {
            ImGui::Separator();
            ImGui::Columns(2, current_category->title, true);
            for (int i = 0; i < current_category->games.size(); i++)
            {
                Game *game = &current_category->games[i];
                ImGui::SetColumnWidth(-1, 760);
                ImGui::PushID(i);
                if (ImGui::Selectable(game->title, false, ImGuiSelectableFlags_SpanAllColumns))
                    GAME::Launch(game->id);
                ImGui::PopID();
                if (position == i)
                {
                    SetNavFocusHere();
                    ImGui::SetScrollHereY(scroll_direction);
                    position = -1;
                }
                if (ImGui::IsItemHovered())
                    selected_game = game;
                if (game->favorite)
                {
                    ImGui::SameLine();
                    ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
                }
                ImGui::NextColumn();
                ImGui::Text(game->id);
                ImGui::NextColumn();               
                ImGui::Separator();
            }
            ImGui::Columns(1);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ShowSettingsDialog()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        if (io.NavInputs[ImGuiNavInput_Input] == 1.0f)
        {
            paused = true;
            ImGui::OpenPopup("Settings and Actions");
        }

        ImGui::SetNextWindowPos(ImVec2(330, 220));
        if (ImGui::BeginPopupModal("Settings and Actions"))
        {
            static bool refresh_games = false;
            static bool remove_from_cache = false;
            static bool add_new_game = false;
            ImGui::Text("%s View:", current_category->title);
            ImGui::Text("    ");
            ImGui::SameLine();
            ImGui::RadioButton("Grid", &view_mode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("List", &view_mode, 1);
            if (!refresh_games && !add_new_game)
            {
                ImGui::Separator();
                ImGui::Checkbox("Remove selected game from cache", &remove_from_cache);
            }
            if (!refresh_games && !remove_from_cache)
            {
                ImGui::Separator();
                ImGui::Checkbox("Add new game to cache", &add_new_game);
            }
            if (!remove_from_cache && !add_new_game)
            {
                ImGui::Separator();
                ImGui::Checkbox("Rescan games and rebuild cache", &refresh_games);
            }
            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                if (view_mode != current_category->view_mode)
                {
                    current_category->view_mode = view_mode;
                    OpenIniFile (CONFIG_INI_FILE);
                    WriteInt(current_category->title, CONFIG_VIEW_MODE, view_mode);
                    WriteIniFile(CONFIG_INI_FILE);

                    if (view_mode == VIEW_MODE_GRID)
                    {
                        GAME::StartLoadImagesThread(current_category->id, current_category->page_num, current_category->page_num);
                    }
                }

                if (refresh_games)
                    GAME::RefreshGames();

                if (remove_from_cache)
                {
                    if (selected_game != nullptr)
                    {
                        GAME::RemoveGameFromCache(selected_game->id);
                        selected_game = nullptr;
                    }
                }

                if (add_new_game)
                    handle_add_game = true;

                paused = false;
                refresh_games = false;
                remove_from_cache = false;
                add_new_game = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                paused = false;
                refresh_games = false;
                remove_from_cache = false;
                add_new_game = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleLaunchError()
    {
        paused = true;
        ImGui::OpenPopup("Warning!");
        ImGui::SetNextWindowPos(ImVec2(320, 220));
        if (ImGui::BeginPopupModal("Warning!"))
        {
            ImGui::Text("The selected game %s no longer exists.", selected_game->id);
            ImGui::Text("Would you like to remove it from the cache?");
            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                GAME::RemoveGameFromCache(selected_game->id);
                selected_game = nullptr;
                paused = false;
                launch_success = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                paused = false;
                launch_success = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleAddNewGame()
    {
        paused = true;
        static Game game;
        static GameCategory *category = nullptr;

        if (!game_added)
        {
            ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE);
            ImGui::OpenPopup("Select game");
            ImGui::SetNextWindowPos(ImVec2(230, 100));
            ImGui::SetNextWindowSize(ImVec2(490,320));
            if (ImGui::BeginPopupModal("Select game", nullptr, ImGuiWindowFlags_NoScrollbar))
            {
                if (games_on_filesystem.size() == 0)
                {
                    games_on_filesystem = FS::ListDir("ux0:app");
                    std::sort(games_on_filesystem.begin(), games_on_filesystem.end());
                }
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,250));
                ImGui::BeginChild("game list");
                ImGui::Columns(4, "game list", true);
                for (int i = 0; i < games_on_filesystem.size(); i++)
                {
                    ImGui::SetColumnWidth(-1, 120);
                    if (ImGui::Selectable(games_on_filesystem[i].c_str()))
                    {
                        if (GAME::GetGameDetails(games_on_filesystem[i].c_str(), &game))
                        {
                            if (strcmp(game.category, vita.c_str()) == 0)
                            {
                                category = &game_categories[VITA_GAMES];
                            } 
                            else if (strcmp(game.category, psp.c_str()) == 0)
                            {
                                category = &game_categories[PSP_GAMES];
                            }
                            else
                            {
                                category = &game_categories[HOMEBREWS];
                            }

                            if (GAME::FindGamePosition(category, game.id) < 0)
                            {
                                category->games.push_back(game);
                                GAME::SortGames(category);
                                GAME::SetMaxPage(category);
                                GAME::SaveGamesCache();
                            }
                            game_added = true;
                            ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            paused = false;
                            handle_add_game = false;
                            game_added = false;
                            ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
                            ImGui::CloseCurrentPopup();
                        }
                        
                    }
                    ImGui::NextColumn();
                    if (i % 4 == 0)
                    {
                        ImGui::Separator();
                    }
                }
                ImGui::Columns(1);
                ImGui::EndChild();

                ImGui::Separator();
                if (ImGui::Button("Cancel"))
                {
                    paused = false;
                    handle_add_game = false;
                    game_added = false;
                    ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::OpenPopup("Info");
            ImGui::SetNextWindowPos(ImVec2(250, 220));
            if (ImGui::BeginPopupModal("Info"))
            {
                ImGui::PushTextWrapPos(400);
                ImGui::Text("The game \"%s\" has being added to the cache. Please browse to the \"%s\" category to find your game.", game.title, category->title);
                ImGui::PopTextWrapPos();
                ImGui::Separator();
                if (ImGui::Button("OK"))
                {
                    category = nullptr;
                    game_added = false;
                    paused = false;
                    handle_add_game = false;
                    ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
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
            ImGui::SetCursorPos(ImVec2(210, 230));
            ImGui::Text("Scanning games and creating cache in folder ux0:data/SMLA00001");
            ImGui::SetCursorPos(ImVec2(210, 260));
            ImGui::ProgressBar(progress, ImVec2(530, 0));
            ImGui::SetCursorPos(ImVec2(210, 290));
            ImGui::Text("Adding %s to cache", game_scan_inprogress.id);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

}
