#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <algorithm> 
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"
#include "db.h"
#include "config.h"
extern "C" {
	#include "inifile.h"
}

Game *selected_game;
static SceCtrlData pad_prev;
bool paused = false;
int view_mode;
static std::vector<std::string> games_on_filesystem;
static float scroll_direction = 0.0f;

bool handle_add_game = false;
bool game_added = false;
bool handle_boot_game = false;
char game_added_message[256];

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
                        DB::InsertFavorite(nullptr, selected_game);
                    }
                    else {
                        selected_game->favorite = false;
                        int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        if (index != -1)
                            DB::DeleteFavorite(nullptr, selected_game);
                    }
                }
                else
                {
                    Game* game = nullptr;
                    for (int i=1; i<TOTAL_CATEGORY; i++)
                    {
                        game = GAME::FindGame(&game_categories[i], selected_game);
                        if (game != nullptr)
                        {
                            break;
                        }
                    }
                    if (game != nullptr)
                    {
                        game->favorite = false;
                        int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        if (index != -1)
                            DB::DeleteFavorite(nullptr, selected_game);
                        selected_game = nullptr;
                    }
                }
            }
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

        if (ImGui::Begin("Games", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()-7);
            ShowTabBar();
            if (current_category->view_mode == VIEW_MODE_GRID)
            {
                ShowGridViewWindow();
            }
            else if (current_category->view_mode == VIEW_MODE_LIST)
            {
                ShowListViewWindow();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ShowGridViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.05f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(50000);

        int game_start_index = (current_category->page_num * 18) - 18;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-3);
        if (selected_game != nullptr)
        {
            if (selected_game->type == TYPE_BUBBLE)
            {
                ImGui::Text("%s - %s", selected_game->id, selected_game->title);
            }
            else
            {
                ImGui::Text("%s", selected_game->title);
            }
            
        }
        else
        {
            ImGui::Text("No game selected");
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
        ImGui::Separator();
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
                    if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(game->tex.id), ImVec2(138,127), ImVec2(0,0), ImVec2(1,1))) {
                        if (game->type < TYPE_PSP_ISO)
                        {
                            GAME::Launch(game, nullptr);
                        }
                        else
                        {
                            handle_boot_game = true;
                        }
                    }
                    if (ImGui::IsItemFocused())
                    {
                        selected_game = game;
                    }
                    ImGui::PopID();

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
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
        ImGui::SetCursorPos(ImVec2(pos.x, 521));
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
        ImGui::Text("Page: %d/%d", current_category->page_num, current_category->max_page); ImGui::SameLine();
        ImGui::SetCursorPosX(350);
        ImGui::Image(reinterpret_cast<ImTextureID>(circle_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Un-Select"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(square_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Favorite"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(triangle_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Settings"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(cross_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Select"); ImGui::SameLine();

        if (handle_add_game)
        {
            HandleAddNewGame();
        }

        if (handle_boot_game)
        {
            HandleAdernalineGame();
        }

        ShowSettingsDialog();
    }

    void ShowListViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.005f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(1000);

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        {
            if (io.NavInputs[ImGuiNavInput_DpadRight] == 1.0f)
            {
                if (selected_game != nullptr)
                {
                    current_category->list_view_position = GAME::FindGamePosition(current_category, selected_game);
                    current_category->list_view_position += 5;
                    if (current_category->list_view_position > current_category->games.size()-1)
                    {
                        current_category->list_view_position = current_category->games.size()-1;
                    }
                    scroll_direction = 1.0f;
                }
            } else if (io.NavInputs[ImGuiNavInput_DpadLeft] == 1.0f)
            {
                if (selected_game != nullptr)
                {
                    current_category->list_view_position = GAME::FindGamePosition(current_category, selected_game);
                    current_category->list_view_position -= 5;
                    if (current_category->list_view_position < 0)
                    {
                        current_category->list_view_position = 0;
                    }
                    scroll_direction = 0.0f;
                }
            }
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5);
        ImGui::BeginChild(ImGui::GetID(current_category->title), ImVec2(950,480));
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetWindowFocus();
        }
        ImGui::Separator();
        ImGui::Columns(2, current_category->title, true);
        for (int i = 0; i < current_category->games.size(); i++)
        {
            Game *game = &current_category->games[i];
            ImGui::SetColumnWidth(-1, 760);
            ImGui::PushID(i);
            if (ImGui::Selectable(game->title, false, ImGuiSelectableFlags_SpanAllColumns))
                if (game->type < TYPE_PSP_ISO)
                {
                    GAME::Launch(game, nullptr);
                }
                else
                {
                    handle_boot_game = true;
                }
            ImGui::PopID();
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (current_category->list_view_position == i && !paused)
                {
                    SetNavFocusHere();
                    ImGui::SetScrollHereY(scroll_direction);
                    current_category->list_view_position = -1;
                }
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
        ImGui::EndChild();
        ImGui::SetCursorPosY(520);
        ImGui::Separator();
        ImGui::SetCursorPosX(300);
        ImGui::Image(reinterpret_cast<ImTextureID>(circle_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Un-Select"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(square_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Favorite"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(triangle_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Settings"); ImGui::SameLine();
        ImGui::Image(reinterpret_cast<ImTextureID>(cross_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Select"); ImGui::SameLine();
        
        if (handle_add_game)
        {
            HandleAddNewGame();
        }

        if (handle_boot_game)
        {
            HandleAdernalineGame();
        }

        ShowSettingsDialog();
    }

    void ShowTabBar()
    {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;
        if (ImGui::BeginTabBar("Categories", tab_bar_flags))
        {
            for (int i=0; i<TOTAL_CATEGORY; i++)
            if (game_categories[i].games.size() > 0 || show_all_categories)
            {
                ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.31f, 0.31f, 0.31f, 1.00f));
                if (ImGui::BeginTabItem(game_categories[i].title))
                {
                    GameCategory *previous_category = current_category;
                    if (previous_category->id != game_categories[i].id)
                    {
                        current_category->opened = false;
                        current_category = &game_categories[i];
                        current_category->opened = true;
                        view_mode = current_category->view_mode;
                        selected_game = nullptr;

                        GAME::DeleteGamesImages(previous_category);
                        GAME::StartLoadImagesThread(current_category->id, current_category->page_num, current_category->page_num);
                    }
                    ImGui::EndTabItem();
                }
                ImGui::PopStyleColor();
                
            }
            ImGui::EndTabBar();
        }
    }

    void ShowSettingsDialog()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        if (io.NavInputs[ImGuiNavInput_Input] == 1.0f)
        {
            paused = true;
            ImGui::OpenPopup("Settings and Actions");
        }

        ImGui::SetNextWindowPos(ImVec2(300, 180));
        ImGui::SetNextWindowSizeConstraints(ImVec2(400,150), ImVec2(400,400), NULL, NULL);
        if (ImGui::BeginPopupModal("Settings and Actions", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static bool refresh_games = false;
            static bool remove_from_cache = false;
            static bool add_new_game = false;

            ImGui::Text("Global Settings");
            ImGui::Checkbox("Show All Categories", &show_all_categories);
            ImGui::Separator();
            ImGui::Text("%s View:", current_category->title);
            ImGui::RadioButton("Grid", &view_mode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("List", &view_mode, 1);

            if (!parental_control)
            {
                if (GAME::IsRomCategory(current_category->id))
                {
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
                }
                if (!remove_from_cache && !add_new_game)
                {
                    ImGui::Separator();
                    ImGui::Checkbox("Rescan games and rebuild cache", &refresh_games);
                }
            }
            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                OpenIniFile (CONFIG_INI_FILE);
                WriteInt(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, show_all_categories);
                if (view_mode != current_category->view_mode)
                {
                    current_category->view_mode = view_mode;
                    WriteInt(current_category->title, CONFIG_VIEW_MODE, view_mode);

                    if (view_mode == VIEW_MODE_GRID)
                    {
                        GAME::StartLoadImagesThread(current_category->id, current_category->page_num, current_category->page_num);
                    }
                }
                WriteIniFile(CONFIG_INI_FILE);
                CloseIniFile();

                if (refresh_games)
                    GAME::RefreshGames();

                if (remove_from_cache)
                {
                    if (selected_game != nullptr)
                    {
                        GAME::RemoveGameFromCategory(current_category, selected_game);
                        GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        DB::DeleteGame(nullptr, selected_game);
                        DB::DeleteFavorite(nullptr, selected_game);
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
            ImGui::EndPopup();
        }
    }

    void HandleAdernalineGame()
    {
        paused = true;

        ImGui::OpenPopup("Boot Adernaline Game");
        ImGui::SetNextWindowPos(ImVec2(230, 100));
        ImGui::SetNextWindowSize(ImVec2(490,350));
        if (ImGui::BeginPopupModal("Boot Adernaline Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            static BootSettings settings = defaul_boot_settings;

            float posX = ImGui::GetCursorPosX();
            ImGui::Text("Boot Settings");
            ImGui::Separator();
            ImGui::Text("Driver:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("Inferno", settings.driver == INFERNO)) { settings.driver = INFERNO; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("March33", settings.driver == MARCH33)) { settings.driver = MARCH33; } ImGui::SameLine();
            if (ImGui::RadioButton("NP9660", settings.driver == NP9660)) { settings.driver = NP9660; }

            ImGui::Text("Execute:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("eboot.bin", settings.execute == EBOOT_BIN)) { settings.execute = EBOOT_BIN; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("boot.bin", settings.execute == BOOT_BIN)) { settings.execute = BOOT_BIN; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("eboot.old", settings.execute == EBOOT_OLD)) { settings.execute = EBOOT_OLD; }

            ImGui::Text("PS Button Mode:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 150);
            if (ImGui::RadioButton("Menu", settings.ps_button_mode == MENU)) { settings.ps_button_mode = MENU; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("LiveArea", settings.ps_button_mode == LIVEAREA)) { settings.ps_button_mode = LIVEAREA; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("Standard", settings.ps_button_mode == STANDARD)) { settings.ps_button_mode = STANDARD; }

            ImGui::Text("Suspend Threads:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 150);
            if (ImGui::RadioButton("Yes", settings.suspend_threads == SUSPEND_YES)) { settings.suspend_threads = SUSPEND_YES; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("No", settings.suspend_threads == SUSPEND_NO)) { settings.suspend_threads = SUSPEND_NO; }

            ImGui::Text("Plugins:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("Default##plugins", settings.plugins == PLUGINS_DEFAULT)) { settings.plugins = PLUGINS_DEFAULT; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("Enable##plugins", settings.plugins == PLUGINS_ENABLE)) { settings.plugins = PLUGINS_ENABLE; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("Disable##plugins", settings.plugins == PLUGINS_DISABLE)) { settings.plugins = PLUGINS_DISABLE; }

            ImGui::Text("NoNpDrm:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("Default##nonpdrm", settings.nonpdrm == NONPDRM_DEFAULT)) { settings.nonpdrm = NONPDRM_DEFAULT; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("Enable##nonpdrm", settings.nonpdrm == NONPDRM_ENABLE)) { settings.nonpdrm = NONPDRM_ENABLE; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("Disable##nonpdrm", settings.nonpdrm == NONPDRM_DISABLE)) { settings.nonpdrm = NONPDRM_DISABLE; }

            ImGui::Text("High Memory:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("Default##highmem", settings.high_memory == HIGH_MEM_DEFAULT)) { settings.high_memory = HIGH_MEM_DEFAULT; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("Enable##highmem", settings.high_memory == HIGH_MEM_ENABLE)) { settings.high_memory = HIGH_MEM_ENABLE; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("Disable##highmem", settings.high_memory == HIGH_MEM_DISABLE)) { settings.high_memory = HIGH_MEM_DISABLE; }

            ImGui::Text("Cpu Speed:"); ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("Default##cpuspeed", settings.cpu_speed == CPU_DEFAULT)) { settings.cpu_speed = CPU_DEFAULT; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("333/166", settings.cpu_speed == CPU_333_166)) { settings.cpu_speed = CPU_333_166; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("300/150", settings.cpu_speed == CPU_300_150)) { settings.cpu_speed = CPU_300_150; }
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("288/144", settings.cpu_speed == CPU_288_144)) { settings.cpu_speed = CPU_288_144; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("222/111", settings.cpu_speed == CPU_222_111)) { settings.cpu_speed = CPU_222_111; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("200/100", settings.cpu_speed == CPU_200_100)) { settings.cpu_speed = CPU_200_100; }
            ImGui::SetCursorPosX(posX + 110);
            if (ImGui::RadioButton("166/83", settings.cpu_speed == CPU_166_83)) { settings.cpu_speed = CPU_166_83; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 220);
            if (ImGui::RadioButton("100/50", settings.cpu_speed == CPU_100_50)) { settings.cpu_speed = CPU_100_50; } ImGui::SameLine();
            ImGui::SetCursorPosX(posX + 320);
            if (ImGui::RadioButton("133/66", settings.cpu_speed == CPU_133_66)) { settings.cpu_speed = CPU_133_66; } ImGui::SameLine();
            if (ImGui::RadioButton("50/25", settings.cpu_speed == CPU_50_25)) { settings.cpu_speed = CPU_50_25; }


            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                paused = false;
                handle_boot_game = false;
                GAME::Launch(selected_game, &settings);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                paused = false;
                settings = defaul_boot_settings;
                handle_boot_game = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void HandleAddNewGame()
    {
        paused = true;
        static Game game;

        if (!game_added)
        {
            ImGui::OpenPopup("Select game");
            ImGui::SetNextWindowPos(ImVec2(230, 100));
            ImGui::SetNextWindowSize(ImVec2(490,330));
            if (ImGui::BeginPopupModal("Select game", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
            {
                if (games_on_filesystem.size() == 0)
                {
                    games_on_filesystem = FS::ListDir(current_category->roms_path);
                    for (std::vector<std::string>::iterator it=games_on_filesystem.begin(); 
                        it!=games_on_filesystem.end(); )
                    {
                        int index = it->find_last_of(".");
                        if (it->substr(index) == ".png")
                        {
                            it = games_on_filesystem.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    std::sort(games_on_filesystem.begin(), games_on_filesystem.end());
                }
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,260));
                ImGui::BeginChild("game list");
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetWindowFocus();
                }

                for (int i = 0; i < games_on_filesystem.size(); i++)
                {
                    if (ImGui::Selectable(games_on_filesystem[i].c_str()))
                    {
                        sprintf(game.id, "%s", current_category->title);
                        game.type = TYPE_ROM;
                        sprintf(game.category, "%s", current_category->category);
                        sprintf(game.rom_path, "%s/%s", current_category->roms_path, games_on_filesystem[i].c_str());
                        int index = games_on_filesystem[i].find_last_of(".");
                        sprintf(game.title, "%s", games_on_filesystem[i].substr(0, index).c_str());
                        game.tex = no_icon;

                        sprintf(game_added_message, "The game already exists in the cache.");
                        if (!DB::GameExists(nullptr, &game))
                        {
                            current_category->games.push_back(game);
                            DB::InsertGame(nullptr, &game);
                            GAME::SortGames(current_category);
                            GAME::SetMaxPage(current_category);
                            sprintf(game_added_message, "The game has being added to the cache.");
                        }
                        game_added = true;
                        games_on_filesystem.clear();
                    }
                    ImGui::Separator();
                }
                ImGui::EndChild();
                ImGui::SetItemDefaultFocus();

                ImGui::Separator();
                if (ImGui::Button("Cancel"))
                {
                    games_on_filesystem.clear();
                    paused = false;
                    handle_add_game = false;
                    game_added = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::OpenPopup("Info");
            ImGui::SetNextWindowPos(ImVec2(250, 220));
            if (ImGui::BeginPopupModal("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::PushTextWrapPos(400);
                ImGui::Text("%s", game_added_message);
                ImGui::PopTextWrapPos();
                ImGui::Separator();
                if (ImGui::Button("OK"))
                {
                    game_added = false;
                    paused = false;
                    handle_add_game = false;
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
            if (current_category->games.size() > 0 && games_to_scan > 0)
            {
                progress = (float)games_scanned / (float)games_to_scan;
            }
            ImGui::SetCursorPos(ImVec2(210, 230));
            ImGui::Text("%s", scan_message);
            ImGui::SetCursorPos(ImVec2(210, 260));
            ImGui::ProgressBar(progress, ImVec2(530, 0));
            ImGui::SetCursorPos(ImVec2(210, 290));
            ImGui::Text("Adding %s", game_scan_inprogress.title);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

}
