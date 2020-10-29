#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <algorithm> 
#include "windows.h"
#include "textures.h"
#include "fs.h"
#include "game.h"
#include "db.h"
#include "style.h"
#include "config.h"
#include "ime_dialog.h"
#include "gui.h"
//#include "debugnet.h"
extern "C" {
	#include "inifile.h"
}

Game *selected_game;
static SceCtrlData pad_prev;
bool paused = false;
int view_mode;
static std::vector<std::string> games_on_filesystem;
static float scroll_direction = 0.0f;
static int game_position = 0;
static bool tab_infocus = false;
static int category_selected = -1;
static char cb_style_name[64];
static std::vector<std::string> styles;
static ime_callback_t ime_callback = nullptr;
static ime_callback_t ime_after_update = nullptr;
static ime_callback_t ime_before_update = nullptr;
GameCategory *tmp_category;

static std::vector<std::string> *ime_multi_field;
static char* ime_single_field;

bool handle_add_game = false;
bool handle_move_game = false;
bool game_added = false;
bool game_moved = false;
bool handle_boot_game = false;
char game_action_message[256];

float previous_right = 0.0f;
float previous_left = 0.0f;

namespace Windows {
    void Init()
    {
        sprintf(cb_style_name, "%s", style_name);
        // Retrieve styles
        std::vector<std::string> style_files = FS::ListDir(STYLES_FOLDER);
        styles.push_back(CONFIG_DEFAULT_STYLE_NAME);
        for (int i=0; i<style_files.size(); i++)
        {
            int index = style_files[i].find_last_of(".");
            if (index != std::string::npos && style_files[i].substr(index) == ".ini")
            {
                styles.push_back(style_files[i].substr(0, index));
            }
        }
    }

    void HandleLauncherWindowInput()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
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
                        DB::DeleteFavorite(nullptr, selected_game);
                        int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
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
                        DB::DeleteFavorite(nullptr, selected_game);
                        int index = GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        selected_game = nullptr;
                    }
                }
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_LTRIGGER) &&
            !(pad.buttons & SCE_CTRL_LTRIGGER) && !paused)
        {
            GameCategory *next_category = current_category;
            next_category = &game_categories[GAME::DecrementCategory(next_category->id, 1)];
            if (!show_all_categories)
            {
                while (next_category->games.size() == 0)
                {
                    next_category = &game_categories[GAME::DecrementCategory(next_category->id, 1)];
                }
            }
            category_selected = next_category->id;
        } else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) &&
                   !(pad.buttons & SCE_CTRL_RTRIGGER) && !paused)
        {
            GameCategory *next_category = current_category;
            next_category = &game_categories[GAME::IncrementCategory(next_category->id, 1)];
            if (!show_all_categories)
            {
                while (next_category->games.size() == 0)
                {
                    next_category = &game_categories[GAME::IncrementCategory(next_category->id, 1)];
                }
            }
            category_selected = next_category->id;
        }

        if (previous_right == 0.0f &&
            io.NavInputs[ImGuiNavInput_DpadRight] == 1.0f &&
            current_category->view_mode == VIEW_MODE_GRID &&
            current_category->max_page > 1 && !paused && !tab_infocus)
        {
            if (game_position == 5 || game_position == 11 || game_position == 17 ||
                (current_category->page_num == current_category->max_page && 
                game_position == current_category->games.size() - (current_category->page_num*18-18) -1))
            {
                int prev_page = current_category->page_num;
                current_category->page_num = GAME::IncrementPage(current_category->page_num, 1);
                GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->page_num);
                selected_game = nullptr;
            }
        } else if (previous_left == 0.0f &&
            io.NavInputs[ImGuiNavInput_DpadLeft] == 1.0f &&
            current_category->view_mode == VIEW_MODE_GRID &&
            current_category->max_page > 1 && !paused && !tab_infocus)
        {
            if (game_position == 0 || game_position == 6 || game_position == 12)
            {
                int prev_page = current_category->page_num;
                current_category->page_num = GAME::DecrementPage(current_category->page_num, 1);
                GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->page_num);
                selected_game = nullptr;
            }
        }

        pad_prev = pad;
        previous_right = io.NavInputs[ImGuiNavInput_DpadRight];
        previous_left = io.NavInputs[ImGuiNavInput_DpadLeft];
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

    int GetGamePositionOnPage(Game *game)
    {
        for (int i=current_category->page_num*18-18; i < current_category->page_num*18; i++)
        {
            if ((game->type != TYPE_ROM && strcmp(game->id, current_category->games[i].id) == 0) ||
                (game->type == TYPE_ROM && strcmp(game->rom_path, current_category->games[i].rom_path) == 0))
            {
                return i % 18;
            }
        }
    }

    void ShowGridViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.05f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(100000);

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
        ImGuiStyle* style = &ImGui::GetStyle();
        for (int i = 0; i < 3; i++)
        {
            for (int j=0; j < 6; j++)
            {
                ImGui::SetCursorPos(ImVec2(pos.x+(j*160),pos.y+(i*160)));
                int button_id = (i*6)+j;
                if (game_start_index+button_id < current_category->games.size())
                {
                    char id[32];
                    sprintf(id, "%d#image", button_id);
                    Game *game = &current_category->games[game_start_index+button_id];
                    if (ImGui::ImageButtonEx(ImGui::GetID(id), reinterpret_cast<ImTextureID>(game->tex.id), ImVec2(138,127), ImVec2(0,0), ImVec2(1,1), style->FramePadding, ImVec4(0,0,0,0), ImVec4(1,1,1,1))) {
                        if (game->type < TYPE_PSP_ISO)
                        {
                            GAME::Launch(game, nullptr);
                        }
                        else
                        {
                            handle_boot_game = true;
                        }
                    }
                    if (ImGui::IsWindowAppearing() && button_id == 0)
                    {
                        SetNavFocusHere();
                        selected_game = game;
                        game_position = GetGamePositionOnPage(selected_game);
                        tab_infocus = false;
                    }
                    if (ImGui::IsItemFocused())
                    {
                        selected_game = game;
                        game_position = GetGamePositionOnPage(selected_game);
                        tab_infocus = false;
                    }

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
            HandleAdrenalineGame();
        }

        if (handle_move_game)
        {
            HandleMoveGame();
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
            {
                if (game->type < TYPE_PSP_ISO)
                {
                    GAME::Launch(game, nullptr);
                }
                else
                {
                    handle_boot_game = true;
                }
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
            HandleAdrenalineGame();
        }

        if (handle_move_game)
        {
            HandleMoveGame();
        }

        ShowSettingsDialog();
    }

    void ShowTabBar()
    {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll;
        if (ImGui::BeginTabBar("Categories", tab_bar_flags))
        {
            for (int i=0; i<TOTAL_CATEGORY; i++)
            {
                if (game_categories[i].games.size() > 0 || show_all_categories)
                {
                    // Add some padding for title so tabs are consistent width
                    std::string title = std::string(game_categories[i].alt_title);
                    if (title.length() == 2)
                        title = "  " + title + "  ";
                    else if (title.length() == 3)
                        title = " " + title + " ";
                    else if (title == "Vita")
                        title = " " + title + " ";
                    ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_None;
                    if (category_selected == i)
                    {
                        tab_flags = ImGuiTabItemFlags_SetSelected;
                    }
                    if (ImGui::BeginTabItem(title.c_str(), NULL, tab_flags))
                    {
                        GameCategory *previous_category = current_category;
                        if (previous_category->id != game_categories[i].id)
                        {
                            current_category = &game_categories[i];
                            view_mode = current_category->view_mode;
                            selected_game = nullptr;
                            category_selected = -1;

                            GAME::DeleteGamesImages(previous_category);
                            GAME::StartLoadImagesThread(current_category->id, current_category->page_num, current_category->page_num);
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        category_selected = i;
                        tab_infocus = true;
                    }                
                }
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

        ImGui::SetNextWindowPos(ImVec2(250, 140));
        ImGui::SetNextWindowSizeConstraints(ImVec2(430,130), ImVec2(430,400), NULL, NULL);
        if (ImGui::BeginPopupModal("Settings and Actions", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static bool refresh_games = false;
            static bool refresh_current_category = false;
            static bool remove_from_cache = false;
            static bool add_new_game = false;
            static bool move_game = false;
            static bool rename_game = false;

            float posX = ImGui::GetCursorPosX();

            if (ImGui::BeginTabBar("Settings and Actions#tabbar", ImGuiTabBarFlags_FittingPolicyScroll))
            {
                char cat_setting_title[64];
                if (ImGui::BeginTabItem("Category"))
                {
                    ImGui::Text("Title:"); ImGui::SameLine();
                    ImGui::SetCursorPosX(posX + 100);
                    if (ImGui::Selectable(current_category->alt_title, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                    {
                        ime_single_field = current_category->alt_title;
                        ime_before_update = BeforeTitleChangeCallback;
                        ime_after_update = AfterTitleChangeCallback;
                        ime_callback = SingleValueImeCallback;
                        Dialog::initImeDialog("Title", current_category->alt_title, 30, SCE_IME_TYPE_DEFAULT, 0, 0);
                        gui_mode = GUI_MODE_IME;
                        ImGui::CloseCurrentPopup();
                    };
                    if (ImGui::IsWindowAppearing())
                    {
                        SetNavFocusHere();
                    }

                    ImGui::Text("View Mode:"); ImGui::SameLine();
                    ImGui::SetCursorPosX(posX + 100);
                    ImGui::RadioButton("Grid", &view_mode, 0); ImGui::SameLine();
                    ImGui::RadioButton("List", &view_mode, 1);

                    if (current_category->id != FAVORITES && current_category->id != HOMEBREWS)
                    {
                        ImGui::Text("Bubble Titles:  "); ImGui::SameLine();
                        if (ImGui::SmallButton("Add##title_prefixes") && !parental_control)
                        {
                            ime_multi_field = &current_category->valid_title_ids;
                            ime_before_update = nullptr;
                            ime_after_update = nullptr;
                            ime_callback = MultiValueImeCallback;
                            Dialog::initImeDialog("Title Id or Prefix", "", 9, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        }
                        ImGui::SameLine();
                        if (current_category->valid_title_ids.size()>1)
                            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,47));
                        else
                            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,23));
                        ImGui::BeginChild("Title Prefixes");
                        ImGui::Columns(2, "Title Prefixes", true);
                        for (std::vector<std::string>::iterator it=current_category->valid_title_ids.begin(); 
                            it!=current_category->valid_title_ids.end(); )
                        {
                            ImGui::SetColumnWidth(-1,110);
                            if (ImGui::Selectable(it->c_str(), false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                            {
                                ime_multi_field = &current_category->valid_title_ids;
                                ime_before_update = nullptr;
                                ime_after_update = nullptr;
                                ime_callback = MultiValueImeCallback;
                                Dialog::initImeDialog("Title Id or Prefix", it->c_str(), 9, SCE_IME_TYPE_DEFAULT, 0, 0);
                                gui_mode = GUI_MODE_IME;
                            };
                            ImGui::NextColumn();
                            char buttonId[64];
                            sprintf(buttonId, "Delete##%s", it->c_str());
                            if (ImGui::SmallButton(buttonId) && !parental_control)
                            {
                                current_category->valid_title_ids.erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                            
                            ImGui::NextColumn();               
                            ImGui::Separator();
                        }
                        ImGui::Columns(1);
                        ImGui::EndChild();
                    }

                    if (current_category->id == PS1_GAMES || current_category->rom_type == TYPE_ROM)
                    {
                        ImGui::Text("Icon Path:"); ImGui::SameLine();
                        ImGui::SetCursorPosX(posX + 100);
                        ImGui::PushID("icon_path");
                        if (ImGui::Selectable(current_category->icon_path, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                        {
                            ime_single_field = current_category->icon_path;
                            ime_before_update = nullptr;
                            ime_after_update = AfterPathChangeCallback;
                            ime_callback = SingleValueImeCallback;
                            Dialog::initImeDialog("Icon Path", current_category->icon_path, 95, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        };
                        ImGui::PopID();

                        ImGui::PushID("roms_path");
                        ImGui::Text("Roms Path:"); ImGui::SameLine();
                        ImGui::SetCursorPosX(posX + 100);
                        if (ImGui::Selectable(current_category->roms_path, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                        {
                            ime_single_field = current_category->roms_path;
                            ime_before_update = nullptr;
                            ime_after_update = AfterPathChangeCallback;
                            ime_callback = SingleValueImeCallback;
                            Dialog::initImeDialog("Roms Path", current_category->roms_path, 95, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        };
                        ImGui::PopID();

                        if (strcmp(current_category->rom_launcher_title_id, "DEDALOX64") != 0)
                        {
                            ImGui::PushID("retro_core");
                            ImGui::Text("Retro Core:"); ImGui::SameLine();
                            ImGui::SetCursorPosX(posX + 100);
                            if (ImGui::Selectable(current_category->core, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                            {
                                ime_single_field = current_category->core;
                                ime_before_update = nullptr;
                                ime_after_update = nullptr;
                                ime_callback = SingleValueImeCallback;
                                Dialog::initImeDialog("Core Path", current_category->core, 63, SCE_IME_TYPE_DEFAULT, 0, 0);
                                gui_mode = GUI_MODE_IME;
                            }
                            ImGui::PopID();
                        }
                        
                        ImGui::Text("Rom Extensions:"); ImGui::SameLine();
                        if (ImGui::SmallButton("Add##file_filters") && !parental_control)
                        {
                            ime_multi_field = &current_category->file_filters;
                            ime_before_update = nullptr;
                            ime_after_update = nullptr;
                            ime_callback = MultiValueImeCallback;
                            Dialog::initImeDialog("Extension: Include \".\"", "", 5, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        }
                        ImGui::SameLine();
                        if (current_category->file_filters.size()>1)
                            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,47));
                        else
                            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,23));
                        ImGui::BeginChild("Rom Extensions");
                        ImGui::Columns(2, "Rom Extensions", true);
                        for (std::vector<std::string>::iterator it=current_category->file_filters.begin(); 
                            it!=current_category->file_filters.end(); )
                        {
                            ImGui::SetColumnWidth(-1,110);
                            if (ImGui::Selectable(it->c_str(), false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                            {
                                ime_multi_field = &current_category->file_filters;
                                ime_before_update = nullptr;
                                ime_after_update = nullptr;
                                ime_callback = MultiValueImeCallback;
                                Dialog::initImeDialog("Extension: Include \".\"", it->c_str(), 5, SCE_IME_TYPE_DEFAULT, 0, 0);
                                gui_mode = GUI_MODE_IME;
                            };
                            ImGui::NextColumn();
                            char buttonId[64];
                            sprintf(buttonId, "Delete##%s", it->c_str());
                            if (ImGui::SmallButton(buttonId) && !parental_control)
                            {
                                current_category->file_filters.erase(it);
                            }
                            else
                            {
                                ++it;
                            }
                            
                            ImGui::NextColumn();               
                            ImGui::Separator();
                        }
                        ImGui::Columns(1);
                        ImGui::EndChild();
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Global"))
                {
                    ImGui::Checkbox("Show All Categories", &show_all_categories);
                    if (ImGui::IsWindowAppearing())
                    {
                        SetNavFocusHere();
                    }
                    ImGui::Text("Style:"); ImGui::SameLine();
                    if (ImGui::BeginCombo("##Style", cb_style_name, ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightRegular))
                    {
                        for (int n = 0; n < styles.size(); n++)
                        {
                            const bool is_selected = strcmp(styles[n].c_str(), cb_style_name)==0;
                            if (ImGui::Selectable(styles[n].c_str(), is_selected))
                                sprintf(cb_style_name, "%s", styles[n].c_str());

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::PushID("pspemu_location");
                    ImGui::Text("Pspemu Path:"); ImGui::SameLine();
                    if (ImGui::Selectable(pspemu_path, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                    {
                        ime_single_field = pspemu_path;
                        ime_before_update = nullptr;
                        ime_after_update = AfterPspemuChangeCallback;
                        ime_callback = SingleValueImeCallback;
                        Dialog::initImeDialog("Pspemu Path", pspemu_path, 11, SCE_IME_TYPE_DEFAULT, 0, 0);
                        gui_mode = GUI_MODE_IME;
                    }
                    ImGui::PopID();

                    ImGui::Text("Hidden Titles:"); ImGui::SameLine();
                    if (ImGui::SmallButton("Add##hidden_titles") && !parental_control)
                    {
                        ime_multi_field = &hidden_title_ids;
                        ime_before_update = nullptr;
                        ime_after_update = nullptr;
                        ime_callback = MultiValueImeCallback;
                        Dialog::initImeDialog("Title Id", "", 9, SCE_IME_TYPE_DEFAULT, 0, 0);
                        gui_mode = GUI_MODE_IME;
                    }

                    ImGui::SameLine();
                    if (hidden_title_ids.size()>1)
                        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,47));
                    else
                        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(190,23));
                    ImGui::BeginChild("Hidden Titles");
                    ImGui::Columns(2, "Hidden Titles", true);
                    for (std::vector<std::string>::iterator it=hidden_title_ids.begin(); 
                        it!=hidden_title_ids.end(); )
                    {
                        ImGui::SetColumnWidth(-1,110);
                        if (ImGui::Selectable(it->c_str(), false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                        {
                            ime_multi_field = &hidden_title_ids;
                            ime_before_update = nullptr;
                            ime_after_update = nullptr;
                            ime_callback = MultiValueImeCallback;
                            Dialog::initImeDialog("Title Id", it->c_str(), 9, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        };
                        ImGui::NextColumn();
                        char buttonId[64];
                        sprintf(buttonId, "Delete##%s", it->c_str());
                        if (ImGui::SmallButton(buttonId) && !parental_control)
                        {
                            hidden_title_ids.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                        
                        ImGui::NextColumn();               
                        ImGui::Separator();
                    }
                    ImGui::Columns(1);
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }

                if (!parental_control)
                {
                    if (ImGui::BeginTabItem("Actions"))
                    {
                        if (selected_game != nullptr && current_category->id != FAVORITES)
                        {
                            if (!refresh_games && !add_new_game && !refresh_current_category && !remove_from_cache &&
                                !move_game && selected_game->type != TYPE_BUBBLE)
                            {
                                ImGui::Checkbox("Rename selected game", &rename_game);
                                ImGui::Separator();
                            }

                            if (!refresh_games && !add_new_game && !refresh_current_category && !remove_from_cache && !rename_game)
                            {
                                ImGui::Checkbox("Move selected game", &move_game);
                                ImGui::Separator();
                            }

                            if (!refresh_games && !add_new_game && !refresh_current_category && !move_game && !rename_game)
                            {
                                ImGui::Checkbox("Hide selected game", &remove_from_cache);
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type == TYPE_ROM || current_category->id == PS1_GAMES)
                        {
                            if (!refresh_games && !remove_from_cache && !refresh_current_category && !move_game && !rename_game)
                            {
                                ImGui::Checkbox("Add new game to cache", &add_new_game);
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type != TYPE_BUBBLE)
                        {
                            if (!refresh_games && !remove_from_cache && !add_new_game && !move_game && !rename_game)
                            {
                                char cb_text[64];
                                sprintf(cb_text, "Rescan games in %s category only", current_category->title);
                                ImGui::Checkbox(cb_text, &refresh_current_category);
                                ImGui::Separator();
                            }
                        }
                        
                        if (!remove_from_cache && !add_new_game && !refresh_current_category && !move_game && !rename_game)
                        {
                            ImGui::Checkbox("Rescan all game categories to rebuild cache", &refresh_games);
                            ImGui::Separator();
                        }
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Help"))
                    {
                        ImGui::Text("1. Title ids can be full 9 characters or partial. A");
                        ImGui::Text("   partial title is used for matching multiple title ids.");
                        ImGui::Text("   App performs better with partial title ids.");
                        ImGui::Text("   Example:");
                        ImGui::Text("     The partial title id \"PSPEMU\" would match all");
                        ImGui::Text("     titles that starts with \"PSPEMU\" like PSPEMU001,");
                        ImGui::Text("     PSPEMU002, PSPEMU003... etc");
                        ImGui::Text("");
                        ImGui::Text("2. Some of the settings takes effect after restarting");
                        ImGui::Text("   the application.");
                        ImGui::Separator();
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }

            if (ImGui::Button("OK"))
            {
                OpenIniFile (CONFIG_INI_FILE);
                WriteInt(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, show_all_categories);
                WriteString(CONFIG_GLOBAL, CONFIG_PSPEMU_PATH, pspemu_path);
                WriteString(CONFIG_GLOBAL, CONFIG_STYLE_NAME, cb_style_name);

                if (remove_from_cache && selected_game != nullptr && selected_game->type == TYPE_BUBBLE)
                {
                    Game *game = selected_game;
                    hidden_title_ids.push_back(selected_game->id);
                    GAME::RemoveGameFromCategory(current_category, game);
                    GAME::RemoveGameFromCategory(&game_categories[FAVORITES], game);
                    selected_game = nullptr;
                }
                WriteString(CONFIG_GLOBAL, CONFIG_HIDE_TITLE_IDS, CONFIG::GetMultiValueString(hidden_title_ids).c_str());
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
                CONFIG::SaveCategoryConfig(current_category);

                if (strcmp(cb_style_name, style_name) != 0)
                {
                    sprintf(style_name, "%s", cb_style_name);
                    Style::SetStylePath(style_name);
                    Style::LoadStyle(style_path);
                }

                if (refresh_games)
                {
                    GAME::RefreshGames(true);
                }

                if (refresh_current_category)
                {
                    GAME::RefreshGames(false);
                }

                if (remove_from_cache && selected_game != nullptr && selected_game->title != TYPE_BUBBLE)
                {
                    Game *game = selected_game;
                    DB::DeleteGame(nullptr, game);
                    DB::DeleteFavorite(nullptr, game);
                    GAME::RemoveGameFromCategory(current_category, game);
                    GAME::RemoveGameFromCategory(&game_categories[FAVORITES], game);
                    selected_game = nullptr;
                }

                if (rename_game && selected_game != nullptr)
                {
                    ime_single_field = selected_game->title;
                    ime_before_update = nullptr;
                    ime_after_update = AfterGameTitleChangeCallback;
                    ime_callback = SingleValueImeCallback;
                    Dialog::initImeDialog("Game Name", selected_game->title, 127, SCE_IME_TYPE_DEFAULT, 0, 0);
                    gui_mode = GUI_MODE_IME;
                }

                if (add_new_game)
                    handle_add_game = true;

                if (move_game)
                    handle_move_game = true;

                paused = false;
                move_game = false;
                refresh_games = false;
                remove_from_cache = false;
                refresh_current_category = false;
                add_new_game = false;
                rename_game = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleAdrenalineGame()
    {
        paused = true;
        char popup_title[64];
        sprintf(popup_title, "Boot %s Game", current_category->alt_title);
        ImGui::OpenPopup(popup_title);
        if (current_category->id == PS1_GAMES && strcmp(current_category->rom_launcher_title_id, RETROARCH_TITLE_ID) == 0)
        {
            ImGui::SetNextWindowPos(ImVec2(300, 200));
            ImGui::SetNextWindowSize(ImVec2(400,100));
        }
        else
        {
            ImGui::SetNextWindowPos(ImVec2(230, 100));
            ImGui::SetNextWindowSize(ImVec2(495,350));
        }
        if (ImGui::BeginPopupModal(popup_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            static BootSettings settings = defaul_boot_settings;

            float posX = ImGui::GetCursorPosX();
            if (current_category->id == PS1_GAMES)
            {
                ImGui::Text("Boot with: "); ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 110);
                if (ImGui::RadioButton("Adrenaline", strcmp(current_category->rom_launcher_title_id, DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID)==0))
                { 
                    sprintf(current_category->rom_launcher_title_id, "%s", DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("RetroArch", strcmp(current_category->rom_launcher_title_id, RETROARCH_TITLE_ID)==0))
                { 
                    sprintf(current_category->rom_launcher_title_id, "%s", RETROARCH_TITLE_ID);
                }
            }
            else
            {
                ImGui::Text("Boot Settings");
            }

            if (current_category->id != PS1_GAMES || strcmp(current_category->rom_launcher_title_id, RETROARCH_TITLE_ID) != 0)
            {
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
            }

            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                if (current_category->id == PS1_GAMES)
                {
                    OpenIniFile(CONFIG_INI_FILE);
                    WriteString(current_category->title, CONFIG_ROM_LAUNCHER_TITLE_ID, current_category->rom_launcher_title_id);
                    WriteIniFile(CONFIG_INI_FILE);
                    CloseIniFile();
                }
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
                        if (index == std::string::npos || !GAME::IsRomExtension(it->substr(index), current_category->file_filters))
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
                        if (strlen(current_category->roms_path) + games_on_filesystem[i].length() + 1 < 192)
                        {
                            sprintf(game.id, "%s", current_category->title);
                            game.type = TYPE_ROM;
                            sprintf(game.category, "%s", current_category->category);
                            sprintf(game.rom_path, "%s/%s", current_category->roms_path, games_on_filesystem[i].c_str());
                            int index = games_on_filesystem[i].find_last_of(".");
                            if (index > 126) index = 126;
                            sprintf(game.title, "%s", games_on_filesystem[i].substr(0, index).c_str());
                            game.tex = no_icon;

                            sprintf(game_action_message, "The game already exists in the cache.");
                            if (!DB::GameExists(nullptr, &game))
                            {
                                current_category->games.push_back(game);
                                DB::InsertGame(nullptr, &game);
                                GAME::SortGames(current_category);
                                GAME::SetMaxPage(current_category);
                                sprintf(game_action_message, "The game has being added to the cache.");
                            }
                        }
                        else
                        {
                            sprintf(game_action_message, "The length of the rom name is too long.");
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
                ImGui::Text("%s", game_action_message);
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

    void HandleMoveGame()
    {
        paused = true;

        if (!game_moved)
        {
            if (selected_game->type == TYPE_ROM)
            {
                sprintf(game_action_message, "Can't move ROM type games. Since they\nare dependent on RetroArch core of \nthe category.");
                game_moved = true;
            }
            else
            {
                ImGui::OpenPopup("Select Category");
                ImGui::SetNextWindowPos(ImVec2(230, 100));
                ImGui::SetNextWindowSize(ImVec2(490,330));
                if (ImGui::BeginPopupModal("Select Category", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
                {
                    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,260));
                    ImGui::BeginChild("category list");
                    if (ImGui::IsWindowAppearing())
                    {
                        ImGui::SetWindowFocus();
                    }

                    for (int i = 1; i < TOTAL_CATEGORY-1; i++)
                    {
                        if (current_category->id != game_categories[i].id)
                        {
                            if (ImGui::Selectable(game_categories[i].alt_title) &&
                                selected_game != nullptr)
                            {
                                if (selected_game->type > TYPE_ROM)
                                {
                                    Game tmp = *selected_game;
                                    sprintf(tmp.category, "%s", game_categories[i].category);
                                    tmp.tex = no_icon;
                                    game_categories[i].games.push_back(tmp);
                                    DB::UpdateGameCategory(nullptr, &tmp);
                                    DB::UpdateFavoritesGameCategoryByRomPath(nullptr, &tmp);
                                    GAME::SortGames(&game_categories[i]);
                                    GAME::SetMaxPage(&game_categories[i]);
                                    GAME::RemoveGameFromCategory(current_category, selected_game);
                                    GAME::SetMaxPage(current_category);
                                    sprintf(game_action_message, "Game moved to %s category", game_categories[i].alt_title);
                                } else if (selected_game->type == TYPE_BUBBLE)
                                {
                                    game_categories[i].valid_title_ids.push_back(selected_game->id);
                                    CONFIG::RemoveFromMultiValues(current_category->valid_title_ids, selected_game->id);
                                    OpenIniFile(CONFIG_INI_FILE);
                                    WriteString(game_categories[i].title, CONFIG_TITLE_ID_PREFIXES, CONFIG::GetMultiValueString(game_categories[i].valid_title_ids).c_str());
                                    WriteString(current_category->title, CONFIG_TITLE_ID_PREFIXES, CONFIG::GetMultiValueString(current_category->valid_title_ids).c_str());
                                    WriteIniFile(CONFIG_INI_FILE);
                                    CloseIniFile();
                                    sprintf(selected_game->category, "%s", game_categories[i].category);
                                    DB::UpdateFavoritesGameCategoryById(nullptr, selected_game);
                                    game_categories[i].games.push_back(*selected_game);
                                    GAME::SortGames(&game_categories[i]);
                                    GAME::SetMaxPage(&game_categories[i]);
                                    GAME::RemoveGameFromCategory(current_category, selected_game);
                                    GAME::SetMaxPage(current_category);
                                    sprintf(game_action_message, "Game moved to %s category", game_categories[i].alt_title);
                                }
                                
                                game_moved = true;
                            }
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                    ImGui::SetItemDefaultFocus();

                    ImGui::Separator();
                    if (ImGui::Button("Cancel"))
                    {
                        paused = false;
                        handle_move_game = false;
                        game_moved = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }
        else
        {
            ImGui::OpenPopup("Info");
            ImGui::SetNextWindowPos(ImVec2(250, 220));
            if (ImGui::BeginPopupModal("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::PushTextWrapPos(400);
                ImGui::Text("%s", game_action_message);
                ImGui::PopTextWrapPos();
                ImGui::Separator();
                if (ImGui::Button("OK"))
                {
                    game_moved = false;
                    paused = false;
                    handle_move_game = false;
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

    void HandleImeInput()
    {
        int ime_result = Dialog::updateImeDialog();

        if (ime_result == IME_DIALOG_RESULT_FINISHED || ime_result == IME_DIALOG_RESULT_CANCELED)
        {
            if (ime_result == IME_DIALOG_RESULT_FINISHED)
            {
                if (ime_before_update != nullptr)
                {
                    ime_before_update(ime_result);
                }

                if (ime_callback != nullptr)
                {
                    ime_callback(ime_result);
                }

                if (ime_after_update != nullptr)
                {
                    ime_after_update(ime_result);
                }
            }

            gui_mode = GUI_MODE_LAUNCHER;
        }
    }

    void SingleValueImeCallback(int ime_result)
    {
        if (ime_result == IME_DIALOG_RESULT_FINISHED)
        {
            char *new_value = (char *)Dialog::getImeDialogInputTextUTF8();
            sprintf(ime_single_field, "%s", new_value);
        }
    }

    void MultiValueImeCallback(int ime_result)
    {
        if (ime_result == IME_DIALOG_RESULT_FINISHED)
        {
            char *new_value = (char *)Dialog::getImeDialogInputTextUTF8();
            char *initial_value = (char *)Dialog::getImeDialogInitialText();
            if (strlen(initial_value) == 0)
            {
                ime_multi_field->push_back(new_value);
            }
            else
            {
                for (int i=0; i < ime_multi_field->size(); i++)
                {
                    if (strcmp((*ime_multi_field)[i].c_str(), initial_value)==0)
                    {
                        (*ime_multi_field)[i] = new_value;
                    }
                }
            }
            
        }
    }

    void NullAfterValueChangeCallback(int ime_result) {}

    void AfterTitleChangeCallback(int ime_result)
    {
        OpenIniFile(CONFIG_INI_FILE);
        WriteString(tmp_category->title, CONFIG_ALT_TITLE, tmp_category->alt_title);
        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void BeforeTitleChangeCallback(int ime_result)
    {
        tmp_category = current_category;
    }

    void AfterPspemuChangeCallback(int ime_result)
    {
        AfterPathChangeCallback(ime_result);
        sprintf(pspemu_iso_path, "%s/ISO", pspemu_path);
        sprintf(pspemu_eboot_path, "%s/PSP/GAME", pspemu_path);
    }

    void AfterGameTitleChangeCallback(int ime_result)
    {
        DB::UpdateGameTitle(nullptr, selected_game);
        Game* game = GAME::FindGame(&game_categories[FAVORITES], selected_game);
        if (game != nullptr)
        {
            sprintf(game->title, "%s", selected_game->title);
        }
    }

    void AfterPathChangeCallback(int ime_result)
    {
        std::string str = std::string(ime_single_field);
        CONFIG::trim(str, " ");
        CONFIG::rtrim(str, "/");
        CONFIG::rtrim(str, " ");
        sprintf(ime_single_field, "%s", str.c_str());
    }
}
