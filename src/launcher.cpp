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
//##include "debugnet.h"
extern "C" {
	#include "inifile.h"
}

typedef struct
{
    char category[10];
    char name[32];
    bool selected = false;
} CategorySelection;

Game *selected_game;
Game *game_to_boot;
static SceCtrlData pad_prev;
bool paused = false;
int view_mode;
int grid_rows;
int aspect_ratio;
static std::vector<std::string> games_on_filesystem;
static float scroll_direction = 0.0f;
static int game_position = 0;
static bool tab_infocus = false;
static int category_selected = -1;
static char cb_style_name[64];
static char cb_category_name[16] = "all";
static int search_count = 0;
static std::vector<std::string> styles;
static ime_callback_t ime_callback = nullptr;
static ime_callback_t ime_after_update = nullptr;
static ime_callback_t ime_before_update = nullptr;
static std::vector<std::string> retro_cores;
static char txt_search_text[32];
static std::vector<Game> games_selection;
static BootSettings settings;
static char retro_core[128];
static int move_location = 0;
static Folder temp_folder;
static std::vector<CategorySelection> categories_selection;

GameCategory *tmp_category;

static std::vector<std::string> *ime_multi_field;
static char* ime_single_field;

bool handle_add_rom_game = false;
bool handle_move_game = false;
bool game_added = false;
bool game_moved = false;
bool handle_boot_game = false;
bool handle_boot_rom_game = false;
bool handle_add_iso_game = false;
bool handle_add_eboot_game = false;
bool handle_search_game = false;
bool handle_uninstall_game = false;
bool handle_new_folder = false;
bool handle_edit_delete_folder = false;
bool selection_mode = false;
bool show_all_categories_setting = show_all_categories;
bool show_categories_as_tabs_settings = show_categories_as_tabs;

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
        sprintf(txt_search_text, search_text);
        grid_rows = current_category->rows;
        aspect_ratio = current_category->ratio;
    }

    void HandleLauncherWindowInput()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        SceCtrlData pad;
        //sceCtrlPeekBufferNegative(0, &pad, 1);
        sceCtrlReadBufferPositiveExt2(0, &pad, 1);

        if ((pad_prev.buttons & SCE_CTRL_SQUARE) && !(pad.buttons & SCE_CTRL_SQUARE) && !paused)
        {
            if (selected_game != nullptr && selected_game->type != TYPE_FOLDER)
            {
                if (current_category->id != FAVORITES)
                {
                    if (!selected_game->favorite)
                    {
                        Game game = *selected_game;
                        game.tex = no_icon;
                        game.visible = false;
                        game.thread_started = false;
                        game_categories[FAVORITES].current_folder->games.push_back(game);
                        GAME::SortGames(&game_categories[FAVORITES]);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        selected_game->favorite = true;
                        DB::InsertFavorite(nullptr, selected_game);
                    }
                    else {
                        selected_game->favorite = false;
                        DB::DeleteFavorite(nullptr, selected_game);
                        GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
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
                        GAME::RemoveGameFromCategory(&game_categories[FAVORITES], selected_game);
                        GAME::SetMaxPage(&game_categories[FAVORITES]);
                        selected_game = nullptr;
                    }
                }
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_L1) &&
            !(pad.buttons & SCE_CTRL_L1) && !paused)
        {
            GameCategory *next_category = current_category;
            next_category = &game_categories[GAME::DecrementCategory(next_category->id, 1)];
            if (!show_all_categories)
            {
                while (next_category->current_folder->games.size() == 0)
                {
                    next_category = &game_categories[GAME::DecrementCategory(next_category->id, 1)];
                }
            }
            category_selected = next_category->id;
            if (!show_categories_as_tabs)
            {
                ChangeCategory(current_category, category_selected);
            }
        }
        else if ((pad_prev.buttons & SCE_CTRL_R1) &&
                   !(pad.buttons & SCE_CTRL_R1) && !paused)
        {
            GameCategory *next_category = current_category;
            next_category = &game_categories[GAME::IncrementCategory(next_category->id, 1)];
            if (!show_all_categories)
            {
                while (next_category->current_folder->games.size() == 0)
                {
                    next_category = &game_categories[GAME::IncrementCategory(next_category->id, 1)];
                }
            }
            category_selected = next_category->id;
            if (!show_categories_as_tabs)
            {
                ChangeCategory(current_category, category_selected);
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_R2) &&
            !(pad.buttons & SCE_CTRL_R2) &&
            current_category->view_mode == VIEW_MODE_GRID && !paused)
        {
			int prev_page = current_category->current_folder->page_num;
			current_category->current_folder->page_num = GAME::IncrementPage(current_category->current_folder->page_num, 1);
			GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->current_folder->page_num, current_category->games_per_page);
			selected_game = nullptr;
        } else if ((pad_prev.buttons & SCE_CTRL_L2) &&
                   !(pad.buttons & SCE_CTRL_L2) &&
                   current_category->view_mode == VIEW_MODE_GRID && !paused)
        {
			int prev_page = current_category->current_folder->page_num;
			current_category->current_folder->page_num = GAME::DecrementPage(current_category->current_folder->page_num, 1);
			GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->current_folder->page_num, current_category->games_per_page);
			selected_game = nullptr;
		}
		
        if (previous_right == 0.0f &&
            io.NavInputs[ImGuiNavInput_DpadRight] == 1.0f &&
            current_category->view_mode == VIEW_MODE_GRID &&
            current_category->current_folder->max_page > 1 && !paused && !tab_infocus)
        {
            if (((game_position == 5 || game_position == 11 || game_position == 17) && current_category->rows == 3) ||
                ((game_position == 3 || game_position == 7) && current_category->rows == 2) ||
                (current_category->current_folder->page_num == current_category->current_folder->max_page && 
                game_position == current_category->current_folder->games.size() - (current_category->current_folder->page_num*current_category->games_per_page-current_category->games_per_page) -1))
            {
                int prev_page = current_category->current_folder->page_num;
                current_category->current_folder->page_num = GAME::IncrementPage(current_category->current_folder->page_num, 1);
                GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->current_folder->page_num, current_category->games_per_page);
                selected_game = nullptr;
            }
        } else if (previous_left == 0.0f &&
            io.NavInputs[ImGuiNavInput_DpadLeft] == 1.0f &&
            current_category->view_mode == VIEW_MODE_GRID &&
            current_category->current_folder->max_page > 1 && !paused && !tab_infocus)
        {
            if (((game_position == 0 || game_position == 6 || game_position == 12) && current_category->rows == 3) ||
                ((game_position == 0 || game_position == 4) && current_category->rows == 2))
            {
                int prev_page = current_category->current_folder->page_num;
                current_category->current_folder->page_num = GAME::DecrementPage(current_category->current_folder->page_num, 1);
                GAME::StartLoadImagesThread(current_category->id, prev_page, current_category->current_folder->page_num, current_category->games_per_page);
                selected_game = nullptr;
            }
        }

        if ((pad_prev.buttons & SCE_CTRL_START) && !(pad.buttons & SCE_CTRL_START) && !paused)
        {
            handle_search_game = true;
            search_count++;
        }

        if ((pad_prev.buttons & SCE_CTRL_SELECT) && !(pad.buttons & SCE_CTRL_SELECT) && !paused)
        {
            if (selection_mode)
            {
                GAME::ClearSelection(current_category);
            }
            selection_mode = !selection_mode;
        }

        if (!show_categories_as_tabs)
        {
            if ((pad_prev.buttons & SCE_CTRL_CIRCLE) && !(pad.buttons & SCE_CTRL_CIRCLE) && !paused)
            {
                if (current_category->current_folder != &current_category->folders[0])
                {
                    ChangeToRootFolder();
                }
                else if (!show_categories_as_tabs && current_category->id != CATEGORY)
                {
                    ChangeCategory(current_category, CATEGORY);
                }
            }
        }

        pad_prev = pad;
        previous_right = io.NavInputs[ImGuiNavInput_DpadRight];
        previous_left = io.NavInputs[ImGuiNavInput_DpadLeft];
    }

    void DeleteGameFromCache(sqlite3 *cache_db, sqlite3 *vita_db, Game *game)
    {
        if (game->type == TYPE_BUBBLE)
        {
            hidden_title_ids.push_back(game->id);
            DB::DeleteVitaAppFolderById(vita_db, game->id);
        }
        else
        {
            DB::DeleteGame(cache_db, game);
            DB::DeleteFavorite(cache_db, game);
        }
        GAME::RemoveGameFromCategory(categoryMap[game->category], game);
        GAME::RemoveGameFromCategory(&game_categories[FAVORITES], game);
    }

    void ChangeCategory(GameCategory *previous_category, int category_id)
    {
        selection_mode = false;
        GAME::ClearSelection(previous_category);
        current_category = &game_categories[category_id];
        view_mode = current_category->view_mode;
        category_selected = -1;
        selected_game = nullptr;
        grid_rows = current_category->rows;
        aspect_ratio = current_category->ratio;

        if (previous_category->id != CATEGORY)
        {
            GAME::StartDeleteGameImagesThread(previous_category, 5000);
        }

        if(current_category->view_mode == VIEW_MODE_GRID)
        {
            GAME::StartLoadImagesThread(current_category->id, current_category->current_folder->page_num, current_category->current_folder->page_num, current_category->games_per_page);
        }
    }

    void ChangeToRootFolder()
    {
        if (current_category->current_folder != &current_category->folders[0])
        {
            if (selection_mode)
            {
                selection_mode = false;
                GAME::ClearSelection(current_category);
            }

            if (current_category->view_mode == VIEW_MODE_LIST)
            {
                current_category->current_folder = &current_category->folders[0];
                selected_game = nullptr;
                current_category->list_view_position = 0;
                if (show_categories_as_tabs)
                {
                    ImGui::SetNextWindowFocus();
                }
            }
            else if (current_category->view_mode == VIEW_MODE_SCROLL)
            {
                GAME::StartDeleteGameImagesThread(current_category,20000);
                current_category->current_folder = &current_category->folders[0];
                if (show_categories_as_tabs)
                {
                    ImGui::SetNextWindowFocus();
                }
            }
            else
            {
                GAME::StartDeleteGameImagesThread(current_category,20000);
                current_category->current_folder = &current_category->folders[0];
                GAME::StartLoadImagesThread(current_category->id, current_category->current_folder->page_num, current_category->current_folder->page_num, current_category->games_per_page);
            }
        }
    }

    void SetModalMode(bool modal)
    {
        paused = modal;
        if (!show_categories_as_tabs)
        {
            if (modal)
            {
                ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE);
            }
            else
            {
                ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
            }
        }
    }
    void LauncherWindow() {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        if (ImGui::Begin("Games", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()-7);
            if (show_categories_as_tabs)
            {
                ShowTabBar();
            }
            if (current_category->view_mode == VIEW_MODE_GRID)
            {
                ShowGridViewWindow();
            }
            else if (current_category->view_mode == VIEW_MODE_LIST)
            {
                ShowListViewWindow();
            }
            else if (current_category->view_mode == VIEW_MODE_SCROLL)
            {
                ShowScrollViewWindow();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    int GetGamePositionOnPage(Game *game)
    {
        for (int i=current_category->current_folder->page_num*current_category->games_per_page-current_category->games_per_page; i < current_category->current_folder->page_num*current_category->games_per_page; i++)
        {
            if ((game->type != TYPE_ROM && game->type != TYPE_SCUMMVM && strcmp(game->id, current_category->current_folder->games[i].id) == 0) ||
                ((game->type == TYPE_ROM || game->type == TYPE_SCUMMVM) && strcmp(game->rom_path, current_category->current_folder->games[i].rom_path) == 0))
            {
                return i % current_category->games_per_page;
            }
        }
    }

    void ShowDisplayTitle()
    {
        if (show_categories_as_tabs)
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()-3);
        }
        else
        {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+6);
        }
        if (!show_categories_as_tabs)
        {
            ImGui::Text(current_category->alt_title);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
        }
        if (current_category->current_folder->type == FOLDER_TYPE_ROOT)
        {
            ImGui::Text(current_category->current_folder->title);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
        }
        else
        {
            if (show_categories_as_tabs)
            {
                char id[32];
                sprintf(id, "../#%d%s", 0, current_category->category);
                if (ImGui::Selectable(id, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(14, 0)))
                {
                    ChangeToRootFolder();
                }
            }
            else
            {
                ImGui::Text("/");
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
            ImGui::Text(current_category->current_folder->title);
            ImGui::SameLine();
        }
        
        char title_text[192];
        if (selected_game != nullptr)
        {
            if (selected_game->type == TYPE_BUBBLE)
            {
                sprintf(title_text, "%s - %s", selected_game->id, selected_game->title);
            }
            else
            {
                sprintf(title_text, "%s", selected_game->title);
            }
        }
        else
        {
            sprintf(title_text, "No game selected");
        }
        
        ImVec2 size = ImGui::CalcTextSize(title_text);
        int x = 470-(size.x/2);
        if (x<0) x = 10;
        ImGui::SetCursorPosX(x);
        if (selected_game != nullptr && selected_game->favorite)
        {
            ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
            ImGui::SameLine();
        }
        ImGui::Text(title_text);
    }

    void ShowGridViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.05f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(100000);

        int game_start_index = (current_category->current_folder->page_num * current_category->games_per_page) - current_category->games_per_page;
        int grid_size = 160;
        if (current_category->rows == 2)
        {
            grid_size = 240;
        }
        ShowDisplayTitle();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
        ImGui::Separator();
        ImVec2 pos = ImGui::GetCursorPos();
        ImGuiStyle* style = &ImGui::GetStyle();
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, style->Colors[ImGuiCol_Text]);
        GameCategory *new_category = nullptr;
        Folder *new_folder = nullptr;
        ImVec2 thumbnail_size;
        ImVec2 thumbnail_offset;

        for (int i = 0; i < current_category->rows; i++)
        {
            for (int j=0; j < current_category->columns; j++)
            {
                int button_id = (i*current_category->columns)+j;
                if (game_start_index+button_id < current_category->current_folder->games.size())
                {
                    char id[32];
                    sprintf(id, "%d#image", button_id);
                    Game *game = &current_category->current_folder->games[game_start_index+button_id];
                    if (game->type == TYPE_FOLDER)
                    {
                        thumbnail_offset = ImVec2(0,0);
                        thumbnail_size = current_category->normal_thumbnail_size;
                    }
                    else
                    {
                        thumbnail_size = current_category->thumbnail_size;
                        thumbnail_offset = current_category->thumbnail_offset;
                    }
                    ImGui::SetCursorPos(ImVec2(pos.x+(j*grid_size)+thumbnail_offset.x,pos.y+(i*grid_size)+thumbnail_offset.y));
                    if (ImGui::ImageButtonEx(ImGui::GetID(id), reinterpret_cast<ImTextureID>(game->tex.id), thumbnail_size, ImVec2(0,0), ImVec2(1,1), style->FramePadding, ImVec4(0,0,0,0), ImVec4(1,1,1,1)))
                    {
                        if (game->type == TYPE_CATEGORY)
                        {
                            new_category = categoryMap[game->category];
                        }
                        else if (game->type == TYPE_FOLDER)
                        {
                            if (!selection_mode)
                            {
                                new_folder = GAME::FindFolder(current_category, game->folder_id);
                            }
                        }
                        else if (selection_mode)
                        {
                            game->selected = !game->selected;
                        }
                        else if (game->type == TYPE_BUBBLE || game->type == TYPE_SCUMMVM)
                        {
                            GAME::Launch(game);
                        }
                        else if (game->type == TYPE_ROM)
                        {
                            GameCategory *cat = categoryMap[game->category];
                            if (cat->alt_cores.size() == 0 || !cat->boot_with_alt_core)
                            {
                                GAME::Launch(game);
                            }
                            handle_boot_rom_game = true;
                            game_to_boot = game;
                            sprintf(retro_core, "%s", cat->core);
                            DB::GetRomCoreSettings(game_to_boot->rom_path, retro_core);
                        }
                        else
                        {
                            handle_boot_game = true;
                            game_to_boot = game;
                            settings = defaul_boot_settings;
                            DB::GetPspGameSettings(game_to_boot->rom_path, &settings);
                        }
                    }
                    game->visible = ImGui::IsItemVisible() ? 1 : 0;

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
                    ImGui::SetCursorPosX(pos.x+(j*grid_size));
                    int text_clip = 0;
                    if (game->selected)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(selected_icon.id), ImVec2(16,16));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(pos.x+(j*grid_size)+14);
                        text_clip = 14;
                    }
                    else if (game->favorite)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(pos.x+(j*grid_size)+14);
                        text_clip = 14;
                    }
                    else if (game->type == TYPE_FOLDER)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(folder_icon.id), ImVec2(16,16));
                        ImGui::SameLine();
                        text_clip = 14;
                    }

                    if (current_category->rows == 3)
                    {
                        ImGui::Selectable(game->title, false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_Disabled, ImVec2(135-text_clip, 0));
                    }
                    else
                    {
                        ImGui::Selectable(game->title, false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_Disabled, ImVec2(215-text_clip, 0));
                    }
                }
                if (new_category != nullptr || new_folder != nullptr)
                {
                    break;
                }
            }
            if (new_category != nullptr || new_folder != nullptr)
            {
                break;
            }
        }

        ImGui::PopStyleColor(ImGuiCol_TextDisabled);
        ImGui::SetCursorPos(ImVec2(pos.x, 521));
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
        ImGui::Text("Page: %d/%d", current_category->current_folder->page_num, current_category->current_folder->max_page); ImGui::SameLine();
        ShowCommonSubWindow();

        if (new_category != nullptr)
        {
            ChangeCategory(current_category, new_category->id);
            new_category = nullptr;
            selected_game = nullptr;
        }
        if (new_folder != nullptr)
        {
            GAME::StartDeleteGameImagesThread(current_category, 20000);
            current_category->current_folder = new_folder;
            selected_game = nullptr;
            GAME::StartLoadImagesThread(current_category->id, current_category->current_folder->page_num, current_category->current_folder->page_num, current_category->games_per_page);

        }
    }

    void ShowScrollViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.05f;
        ImGuiStyle* style = &ImGui::GetStyle();
        ImGui_ImplVita2D_SetAnalogRepeatDelay(50000);

        ShowDisplayTitle();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-1);
        ImGui::Separator();
        ImGui::SetNextWindowSizeConstraints(ImVec2(950, 474), ImVec2(950,474));
        char grid_name[64];
        sprintf(grid_name, "%s#grid", current_category->title);
        ImGui::BeginChild(grid_name);
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetWindowFocus();
        }
        ImVec2 pos = ImGui::GetCursorPos();
        ImGui::Columns(current_category->columns, current_category->title, false);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, style->Colors[ImGuiCol_Text]);
        GameCategory *new_category = nullptr;
        Folder *new_folder = nullptr;

        ImVec2 thumbnail_size;
        ImVec2 thumbnail_offset;

        for (int button_id=0; button_id<current_category->current_folder->games.size(); button_id++)
        {
            char id[32];
            char sel_id[32];
            sprintf(id, "%d#image", button_id);
            sprintf(sel_id, "##%d", button_id);
            Game *game = &current_category->current_folder->games[button_id];
            ImGui::BeginGroup();
            ImVec2 pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(pos.x-5, pos.y));
            if (ImGui::Button(sel_id, current_category->button_size))
            {
                if (game->type == TYPE_CATEGORY)
                {
                    new_category = categoryMap[game->category];
                }
                else if (game->type == TYPE_FOLDER)
                {
                    if (!selection_mode)
                    {
                        new_folder = GAME::FindFolder(current_category, game->folder_id);
                    }
                }
                else if (selection_mode)
                {
                    game->selected = !game->selected;
                }
                else if (game->type == TYPE_BUBBLE || game->type == TYPE_SCUMMVM)
                {
                    GAME::Launch(game);
                }
                else if (game->type == TYPE_ROM)
                {
                    GameCategory *cat = categoryMap[game->category];
                    if (cat->alt_cores.size() == 0 || !cat->boot_with_alt_core)
                    {
                        GAME::Launch(game);
                    }
                    handle_boot_rom_game = true;
                    game_to_boot = game;
                    sprintf(retro_core, "%s", cat->core);
                    DB::GetRomCoreSettings(game_to_boot->rom_path, retro_core);
                }
                else
                {
                    handle_boot_game = true;
                    game_to_boot = selected_game;
                    settings = defaul_boot_settings;
                    DB::GetPspGameSettings(game_to_boot->rom_path, &settings);
                }
            }
            if (ImGui::IsItemFocused())
            {
                selected_game = game;
                tab_infocus = false;
            }

            if (game->type == TYPE_FOLDER)
            {
                thumbnail_size = current_category->normal_thumbnail_size;
                thumbnail_offset = ImVec2(0,0);
            }
            else
            {
                thumbnail_offset = current_category->thumbnail_offset;
                thumbnail_size = current_category->thumbnail_size;
            }
            ImGui::SetCursorPos(ImVec2(pos.x+thumbnail_offset.x, pos.y+4+thumbnail_offset.y));
            ImGui::Image(reinterpret_cast<ImTextureID>(game->tex.id), thumbnail_size);
            if (ImGui::IsItemVisible())
            {
                if (game->tex.id == no_icon.id)
                {
                    if (game->visible == 0)
                    {
                        game->visible_time = sceKernelGetProcessTimeWide();
                    }
                    else if (button_id % current_category->columns == 0)
                    {
                        uint64_t current_time = sceKernelGetProcessTimeWide();
                        if (current_time - game->visible_time > 200000 && !game->thread_started)
                        {
                            GAME::StartLoadGameImageThread(current_category->id, button_id, current_category->columns);
                            game->thread_started = true;
                        }
                    }
                }
                game->visible = 1;
            }
            else
            {
                if (game->visible > 0)
                {
                    game->visible_time = sceKernelGetProcessTimeWide();
                }
                game->visible = 0;
                game->thread_started = false;
                uint64_t current_time = sceKernelGetProcessTimeWide();
                if (game->tex.id != no_icon.id && current_time - game->visible_time > 5000)
                {
                    Tex tmp = game->tex;
                    game->tex = no_icon;
                    Textures::Free(&tmp);
                }
            }
            
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()-2);
            int text_clip = 0;
            if (game->selected)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
                ImGui::Image(reinterpret_cast<ImTextureID>(selected_icon.id), ImVec2(16,16));
                ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10);
                text_clip = 10;
            }
            else if (game->favorite)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
                ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
                ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10);
                text_clip = 10;
            }
            else if (game->type == TYPE_FOLDER)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()-5);
                ImGui::Image(reinterpret_cast<ImTextureID>(folder_icon.id), ImVec2(16,16));
                ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX()-10);
                text_clip = 10;
            }

            if (current_category->rows == 3)
            {
                ImGui::Selectable(game->title, false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_Disabled, ImVec2(135-text_clip, 0));
            }
            else
            {
                ImGui::Selectable(game->title, false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_Disabled, ImVec2(215-text_clip, 0));
            }

            ImGui::EndGroup();
            ImGui::NextColumn();

            if (new_category != nullptr || new_folder)
            {
                break;
            }
        }
        ImGui::PopStyleColor(ImGuiCol_TextDisabled);
        ImGui::EndChild();
        ImGui::Columns(1);

        ImGui::SetCursorPos(ImVec2(pos.x, 521));
        ImGui::Separator();
        ShowCommonSubWindow();

        if (new_category != nullptr)
        {
            ChangeCategory(current_category, new_category->id);
            new_category = nullptr;
            selected_game = nullptr;
        }

        if (new_folder != nullptr)
        {
            GAME::StartDeleteGameImagesThread(current_category, 20000);
            current_category->current_folder = new_folder;
            new_folder = nullptr;
            selected_game = nullptr;
        }
    }

    void ShowListViewWindow()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.KeyRepeatRate = 0.005f;
        ImGui_ImplVita2D_SetAnalogRepeatDelay(1000);

        ShowDisplayTitle();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        {
            if (io.NavInputs[ImGuiNavInput_DpadRight] == 1.0f && !paused)
            {
                if (selected_game != nullptr)
                {
                    current_category->list_view_position = GAME::FindGamePosition(current_category, selected_game);
                    current_category->list_view_position += 5;
                    if (current_category->list_view_position > current_category->current_folder->games.size()-1)
                    {
                        current_category->list_view_position = current_category->current_folder->games.size()-1;
                    }
                    scroll_direction = 1.0f;
                }
            } else if (io.NavInputs[ImGuiNavInput_DpadLeft] == 1.0f && !paused)
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
        ImGui::BeginChild(ImGui::GetID(current_category->title), ImVec2(950,452));
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetWindowFocus();
        }
        ImGui::Separator();
        ImGui::Columns(2, current_category->title, true);
        GameCategory *new_category = nullptr;

        for (int i = 0; i < current_category->current_folder->games.size(); i++)
        {
            Game *game = &current_category->current_folder->games[i];
            ImGui::SetColumnWidth(-1, 760);
            if (game->type == TYPE_FOLDER)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(folder_icon.id), ImVec2(16,16));
                ImGui::SameLine();
            }
            if (game->selected)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(selected_icon.id), ImVec2(16,16));
                ImGui::SameLine();
            }
            ImGui::PushID(i);
            bool folder_selected = false;
            if (ImGui::Selectable(game->title, false, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (game->type == TYPE_CATEGORY)
                {
                    new_category = categoryMap[game->category];
                }
                else if (game->type == TYPE_FOLDER)
                {
                    if (!selection_mode)
                    {
                        GameCategory *cat = categoryMap[game->category];
                        Folder *folder = GAME::FindFolder(cat, game->folder_id);
                        cat->current_folder = folder;
                        cat->list_view_position = 0;
                        selected_game = nullptr;
                        folder_selected = true;
                    }
                }
                else if (selection_mode)
                {
                    game->selected = !game->selected;
                }
                else if (game->type == TYPE_BUBBLE || game->type == TYPE_SCUMMVM)
                {
                    GAME::Launch(game);
                }
                else if (game->type == TYPE_ROM)
                {
                    GameCategory *cat = categoryMap[game->category];
                    if (cat->alt_cores.size() == 0 || !cat->boot_with_alt_core)
                    {
                        GAME::Launch(game);
                    }
                    handle_boot_rom_game = true;
                    game_to_boot = game;
                    sprintf(retro_core, "%s", cat->core);
                    DB::GetRomCoreSettings(game_to_boot->rom_path, retro_core);
                    
                }
                else
                {
                    handle_boot_game = true;
                    game_to_boot = selected_game;
                    settings = defaul_boot_settings;
                    DB::GetPspGameSettings(game_to_boot->rom_path, &settings);
                }
            }
            ImGui::PopID();
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (current_category->list_view_position == i && !folder_selected && !paused)
                {
                    SetNavFocusHere();
                    ImGui::SetScrollHereY(scroll_direction);
                    current_category->list_view_position = -1;
                }
            }
            if (ImGui::IsItemHovered() && !folder_selected)
            {
                selected_game = game;
            }
            if (game->favorite)
            {
                ImGui::SameLine();
                ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
            }
            ImGui::NextColumn();
            if (game->type != TYPE_FOLDER)
            {
                ImGui::Text(game->id);
            }
            ImGui::NextColumn();               
            ImGui::Separator();
            
            if (new_category != nullptr)
            {
                break;
            }
        }
        ImGui::Columns(1);
        ImGui::EndChild();
        ImGui::SetCursorPosY(520);
        ImGui::Separator();
        ShowCommonSubWindow();

        if (new_category != nullptr)
        {
            ChangeCategory(current_category, new_category->id);
            new_category = nullptr;
            selected_game = nullptr;
        }
    }

    void ShowCommonSubWindow()
    {
        ImGui::SetCursorPosX(290);
        ImGui::Image(reinterpret_cast<ImTextureID>(circle_icon.id), ImVec2(16,16)); ImGui::SameLine();
        if (!swap_xo)
        {
            ImGui::Text("Un-Select"); ImGui::SameLine();
        }
        else
        {
            ImGui::Text("Select"); ImGui::SameLine();
        }
            ImGui::Image(reinterpret_cast<ImTextureID>(square_icon.id), ImVec2(16,16)); ImGui::SameLine();
            ImGui::Text("Favorite"); ImGui::SameLine();
            ImGui::Image(reinterpret_cast<ImTextureID>(triangle_icon.id), ImVec2(16,16)); ImGui::SameLine();
            ImGui::Text("Settings"); ImGui::SameLine();
            ImGui::Image(reinterpret_cast<ImTextureID>(cross_icon.id), ImVec2(16,16)); ImGui::SameLine();
        if (!swap_xo)
        {
            ImGui::Text("Select"); ImGui::SameLine();
        }
        else
        {
            ImGui::Text("Un-Select"); ImGui::SameLine();
        }

        ImGui::Image(reinterpret_cast<ImTextureID>(start_icon.id), ImVec2(16,16)); ImGui::SameLine();
        ImGui::Text("Search"); ImGui::SameLine();

        if (selection_mode)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+20);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MULTI SELECT MODE");
        }

        if (handle_add_rom_game)
        {
            HandleAddNewRomGame();
        }

        if (handle_add_iso_game)
        {
            HandleAddNewIsoGame();
        }

        if (handle_add_eboot_game)
        {
            HandleAddNewEbootGame();
        }

        if (handle_boot_game)
        {
            HandleAdrenalineGame();
        }

        if (handle_boot_rom_game)
        {
            HandleBootRomGame();
        }

        if (handle_move_game)
        {
            HandleMoveGame();
        }

        if (handle_search_game)
        {
            HandleSearchGame();
        }

        if (handle_uninstall_game)
        {
			HandleUninstallGame();
		}

        if (handle_new_folder)
        {
            HandleAddNewFolder();
        }

        if (handle_edit_delete_folder)
        {
            HandleEditDeleteFolder();
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
                if (game_categories[i].folders[0].games.size() > 0 || show_all_categories)
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
                            ChangeCategory(previous_category, i);
                        }
                        ImGui::EndTabItem();
                    }
                    if (ImGui::IsItemHovered())
                    {
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
            SetModalMode(true);
            show_categories_as_tabs_settings = show_categories_as_tabs;
            show_all_categories_setting = show_all_categories;
            ImGui::OpenPopup("Settings and Actions");
        }

        if (current_category->rom_type == TYPE_ROM || current_category->id == PS1_GAMES)
        {
            ImGui::SetNextWindowPos(ImVec2(200, 30));
            ImGui::SetNextWindowSizeConstraints(ImVec2(500,130), ImVec2(500,475), NULL, NULL);
        }
        else
        {
            ImGui::SetNextWindowPos(ImVec2(200, 140));
            ImGui::SetNextWindowSizeConstraints(ImVec2(500,130), ImVec2(500,400), NULL, NULL);
        }
        
        if (ImGui::BeginPopupModal("Settings and Actions", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static bool refresh_current_category = false;
            static bool remove_from_cache = false;
            static bool add_rom_game = false;
            static bool move_game = false;
            static bool rename_game = false;
            static bool add_psp_iso_game = false;
            static bool add_eboot_game = false;
            static bool download_thumbnails = false;
            static bool uninstall_game = false;
            static bool add_folder = false;
            static bool edit_folder = false;

            float posX = ImGui::GetCursorPosX();

            if (ImGui::BeginTabBar("Settings and Actions#tabbar", ImGuiTabBarFlags_FittingPolicyScroll))
            {
                if (!parental_control && current_category->id != CATEGORY)
                {
                    if (ImGui::BeginTabItem("Actions"))
                    {
                        if (current_category->id != FAVORITES)
                        {
                            if (!add_rom_game && !refresh_current_category && !remove_from_cache && current_category->current_folder->id == FOLDER_ROOT_ID && !selection_mode
                                && !move_game && !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !uninstall_game && !rename_game && !edit_folder)
                            {
                                ImGui::Checkbox("Add new folder", &add_folder);
                                ImGui::Separator();
                            }
                        }
                        
                        if (selected_game != nullptr && current_category->id != FAVORITES)
                        {
                            if (!add_rom_game && !refresh_current_category && !remove_from_cache && selected_game->type == TYPE_FOLDER && !selection_mode
                                && !move_game && !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !uninstall_game && !rename_game && !add_folder)
                            {
                                ImGui::Checkbox("Edit/Delete folder", &edit_folder);
                                ImGui::Separator();
                            }

                            if (!add_rom_game && !refresh_current_category && !remove_from_cache && !selection_mode &&
                                !move_game && !add_eboot_game && !add_psp_iso_game && selected_game->type != TYPE_BUBBLE && selected_game->type != TYPE_FOLDER
                                && !download_thumbnails && !uninstall_game && !add_folder && !edit_folder)
                            {
                                ImGui::Checkbox("Rename selected game", &rename_game);
                                ImGui::Separator();
                            }

                            if (!add_rom_game && !refresh_current_category && !remove_from_cache && !edit_folder && selected_game->type != TYPE_FOLDER &&
                                !rename_game && !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !uninstall_game && !add_folder)
                            {
                                ImGui::Checkbox("Move selected game", &move_game);
                                ImGui::Separator();
                            }

                            if (!add_rom_game && !refresh_current_category && !move_game && !rename_game && !edit_folder && selected_game->type != TYPE_FOLDER &&
                                !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !uninstall_game && !add_folder)
                            {
                                ImGui::Checkbox("Hide selected game", &remove_from_cache);
                                ImGui::Separator();
                            }

                            if (!add_rom_game && !refresh_current_category && !move_game && !rename_game && !selection_mode &&
                                !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !remove_from_cache && !edit_folder &&
                                selected_game->type == TYPE_BUBBLE && !add_folder)
                            {
                                ImGui::Checkbox("Un-install selected game", &uninstall_game);
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type == TYPE_PSP_ISO)
                        {
                            if (!remove_from_cache && !refresh_current_category && !move_game && !add_folder && !selection_mode &&
                                !add_eboot_game && !add_rom_game && !rename_game && !download_thumbnails && !uninstall_game && !edit_folder)
                            {
                                ImGui::Checkbox("Add new PSP ISO game", &add_psp_iso_game);
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type == TYPE_EBOOT)
                        {
                            if (!remove_from_cache && !refresh_current_category && !move_game && !add_folder && !selection_mode &&
                                !add_psp_iso_game && !add_rom_game && !rename_game && !download_thumbnails && !uninstall_game && !edit_folder)
                            {
                                ImGui::Checkbox("Add new EBOOT game", &add_eboot_game);
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type == TYPE_ROM || current_category->id == PS1_GAMES)
                        {
                            if (!remove_from_cache && !refresh_current_category && !move_game && !add_folder && !selection_mode &&
                                !add_eboot_game && !add_psp_iso_game && !rename_game && !download_thumbnails && !uninstall_game && !edit_folder)
                            {
                                if (current_category->id == PS1_GAMES)
                                {
                                    ImGui::Checkbox("Add new PSX disc format game", &add_rom_game);
                                }
                                else
                                {
                                    ImGui::Checkbox("Add new game to cache", &add_rom_game);
                                }
                                ImGui::Separator();
                            }
                        }

                        if (current_category->rom_type != TYPE_BUBBLE)
                        {
                            if (!remove_from_cache && !add_rom_game && !move_game && !rename_game && !edit_folder && !selection_mode &&
                                !add_eboot_game && !add_psp_iso_game && !download_thumbnails && !uninstall_game && !add_folder)
                            {
                                char cb_text[64];
                                sprintf(cb_text, "Rescan games in %s category only", current_category->title);
                                ImGui::Checkbox(cb_text, &refresh_current_category);
                                ImGui::Separator();
                            }
                        }
                        
                        if (current_category->rom_type == TYPE_ROM || current_category->id == PS1_GAMES
                            || current_category->rom_type == TYPE_SCUMMVM)
                        {
                            if (!remove_from_cache && !refresh_current_category && !move_game && !add_folder && !selection_mode &&
                                !add_eboot_game && !add_psp_iso_game && !rename_game && !add_rom_game && !uninstall_game && !edit_folder)
                            {
                                char cb_text[64];
                                sprintf(cb_text, "Download thumbnails in %s category", current_category->title);
                                ImGui::Checkbox(cb_text, &download_thumbnails);
                                ImGui::Separator();
                            }
                        }

                        ImGui::EndTabItem();
                    }

                }

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
                        SetModalMode(false);
                        ImGui::CloseCurrentPopup();
                    };
                    if (ImGui::IsWindowAppearing())
                    {
                        SetNavFocusHere();
                    }
                    ImGui::Separator();

                    ImGui::Text("View Mode:"); ImGui::SameLine();
                    ImGui::SetCursorPosX(posX + 100);
                    ImGui::RadioButton("Page Grid", &view_mode, 0); ImGui::SameLine();
                    ImGui::RadioButton("Scroll Grid", &view_mode, 2); ImGui::SameLine();
                    ImGui::RadioButton("List", &view_mode, 1);
                    ImGui::Separator();
                    ImGui::Text("Grid Rows:"); ImGui::SameLine();
                    ImGui::RadioButton("2", &grid_rows, 2); ImGui::SameLine();
                    ImGui::RadioButton("3", &grid_rows, 3); ImGui::SameLine();
                    ImGui::Text("Ratio:"); ImGui::SameLine();
                    ImGui::RadioButton("4x4", &aspect_ratio, 1); ImGui::SameLine();
                    ImGui::RadioButton("4x3", &aspect_ratio, 2); ImGui::SameLine();
                    ImGui::RadioButton("3x4", &aspect_ratio, 3);
                    ImGui::Separator();

                    if (current_category->id != CATEGORY)
                    {                
                        ImGui::Text("Category Icon:"); ImGui::SameLine();
                        ImGui::PushID("category_icon");
                        if (ImGui::Selectable(current_category->category_icon, false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                        {
                            ime_single_field = current_category->category_icon;
                            ime_before_update = nullptr;
                            ime_after_update = nullptr;
                            ime_callback = SingleValueImeCallback;
                            Dialog::initImeDialog("Category Icon", current_category->category_icon, 95, SCE_IME_TYPE_DEFAULT, 0, 0);
                            gui_mode = GUI_MODE_IME;
                        };
                        ImGui::PopID();
                        ImGui::Separator();

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
                            ImGui::Separator();
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
                            ImGui::Separator();

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
                            ImGui::Separator();
                            ImGui::Text("Icon Preference:"); ImGui::SameLine();
                            ImGui::RadioButton("Boxart", &current_category->icon_type, 1); ImGui::SameLine();
                            ImGui::RadioButton("Screenshot", &current_category->icon_type, 2);
                            ImGui::Separator();

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
                                ImGui::Separator();

                                ImGui::Text("Alternate Cores:"); ImGui::SameLine();
                                if (ImGui::SmallButton("Add##retro_cores") && !parental_control)
                                {
                                    ime_multi_field = &current_category->alt_cores;
                                    ime_before_update = nullptr;
                                    ime_after_update = nullptr;
                                    ime_callback = MultiValueImeCallback;
                                    Dialog::initImeDialog("Core Path", "", 63, SCE_IME_TYPE_DEFAULT, 0, 0);
                                    gui_mode = GUI_MODE_IME;
                                }
                                ImGui::SameLine();
                                if (current_category->alt_cores.size()>1)
                                    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(300,47));
                                else
                                    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(300,23));
                                ImGui::BeginChild("Alt Cores");
                                ImGui::Columns(2, "Alt Cores", true);
                                for (std::vector<std::string>::iterator it=current_category->alt_cores.begin(); 
                                    it!=current_category->alt_cores.end(); )
                                {
                                    ImGui::SetColumnWidth(-1,220);
                                    if (ImGui::Selectable(it->c_str(), false, ImGuiSelectableFlags_DontClosePopups) && !parental_control)
                                    {
                                        ime_multi_field = &current_category->alt_cores;
                                        ime_before_update = nullptr;
                                        ime_after_update = nullptr;
                                        ime_callback = MultiValueImeCallback;
                                        Dialog::initImeDialog("Core Path", it->c_str(), 63, SCE_IME_TYPE_DEFAULT, 0, 0);
                                        gui_mode = GUI_MODE_IME;
                                    };
                                    ImGui::NextColumn();
                                    char buttonId[64];
                                    sprintf(buttonId, "Delete##%s", it->c_str());
                                    if (ImGui::SmallButton(buttonId) && !parental_control)
                                    {
                                        current_category->alt_cores.erase(it);
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
                                ImGui::Separator();
                                ImGui::Checkbox("Boot with alternate cores", &current_category->boot_with_alt_core);
                                ImGui::Separator();
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
                            ImGui::Separator();
                        }
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Global"))
                {
                    if (show_categories_as_tabs_settings)
                    {
                        ImGui::Checkbox("Show All Categories", &show_all_categories_setting);
                        if (ImGui::IsWindowAppearing())
                        {
                            SetNavFocusHere();
                        }
                        ImGui::Separator();
                    }

                    if (current_category->id != CATEGORY)
                    {
                        ImGui::Checkbox("Show Categories as Tabs", &show_categories_as_tabs_settings);
                        ImGui::Separator();
                    }

                    ImGui::Checkbox("Swap X/O Buttons", &swap_xo);
                    ImGui::Separator();

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
                    ImGui::Separator();

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
                    ImGui::Separator();

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
                    ImGui::Separator();

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

                ImGui::EndTabBar();
            }

            if (ImGui::Button("OK"))
            {
                OpenIniFile (CONFIG_INI_FILE);
                WriteInt(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, show_all_categories_setting);
                WriteBool(CONFIG_GLOBAL, CONFIG_SHOW_CATEGORY_AS_TABS, show_categories_as_tabs_settings);
                WriteBool(CONFIG_GLOBAL, CONFIG_NEW_ICON_METHOD, new_icon_method);
                WriteBool(CONFIG_GLOBAL, CONFIG_SWAP_XO, swap_xo);
                ImGui_ImplVita2D_SwapXO(swap_xo);

                WriteString(CONFIG_GLOBAL, CONFIG_PSPEMU_PATH, pspemu_path);
                WriteString(CONFIG_GLOBAL, CONFIG_STYLE_NAME, cb_style_name);

                if (remove_from_cache && selected_game != nullptr)
                {
                    if (!selection_mode)
                    {
                        DeleteGameFromCache(nullptr, nullptr, selected_game);
                    }
                    else
                    {
                        sqlite3 *cache_db, *vita_db;
                        sqlite3_open(CACHE_DB_FILE, &cache_db);
                        sqlite3_open(VITA_APP_DB_FILE, &cache_db);

                        std::vector<Game> list = GAME::GetSelectedGames(current_category);
                        for (int i=0; i<list.size(); i++)
                        {
                            DeleteGameFromCache(cache_db, vita_db, &list[i]);
                        }
                        sqlite3_close(cache_db);
                        sqlite3_close(vita_db);
                    }
                    GAME::SetMaxPage(current_category);
                    selection_mode = false;
                    GAME::ClearSelection(current_category);
                    selected_game = nullptr;
                }
                WriteString(CONFIG_GLOBAL, CONFIG_HIDE_TITLE_IDS, CONFIG::GetMultiValueString(hidden_title_ids).c_str());
                if (view_mode != current_category->view_mode)
                {
                    current_category->view_mode = view_mode;
                    WriteInt(current_category->title, CONFIG_VIEW_MODE, view_mode);

                    if (view_mode == VIEW_MODE_GRID)
                    {
                        GAME::StartLoadImagesThread(current_category->id, current_category->current_folder->page_num, current_category->current_folder->page_num, current_category->games_per_page);
                    }
                }
                WriteIniFile(CONFIG_INI_FILE);
                CloseIniFile();
                if (grid_rows == 2)
                {
                    current_category->rows = 2;
                    current_category->columns = 4;
                    current_category->button_size = ImVec2(230,233);
                    current_category->normal_thumbnail_size = ImVec2(220,205);
                    current_category->games_per_page = current_category->rows * current_category->columns;
                    if (aspect_ratio == ASPECT_RATIO_4x4)
                    {
                        current_category->thumbnail_size = ImVec2(220,205);
                        current_category->thumbnail_offset = ImVec2(0,0);
                    }
                    else if (aspect_ratio == ASPECT_RATIO_4x3)
                    {
                        current_category->thumbnail_size = ImVec2(220,165);
                        current_category->thumbnail_offset = ImVec2(0,20);
                    }
                    else
                    {
                        current_category->thumbnail_size = ImVec2(154,205);
                        current_category->thumbnail_offset = ImVec2(33,0);
                    }
                }
                else if (grid_rows = 3)
                {
                    current_category->rows = 3;
                    current_category->columns = 6;
                    current_category->button_size = ImVec2(148,154);
                    current_category->normal_thumbnail_size = ImVec2(138,127);
                    current_category->games_per_page = current_category->rows * current_category->columns;
                    if (aspect_ratio == ASPECT_RATIO_4x4)
                    {
                        current_category->thumbnail_size = ImVec2(138,127);
                        current_category->thumbnail_offset = ImVec2(0,0);
                    }
                    else if (aspect_ratio == ASPECT_RATIO_4x3)
                    {
                        current_category->thumbnail_size = ImVec2(138,104);
                        current_category->thumbnail_offset = ImVec2(0,12);
                    }
                    else
                    {
                        current_category->thumbnail_size = ImVec2(95,127);
                        current_category->thumbnail_offset = ImVec2(22,0);
                    }
                }
                current_category->ratio = aspect_ratio;
                GAME::SetMaxPage(current_category);
                CONFIG::SaveCategoryConfig(current_category);

                if (strcmp(cb_style_name, style_name) != 0)
                {
                    sprintf(style_name, "%s", cb_style_name);
                    Style::SetStylePath(style_name);
                    Style::LoadStyle(style_path);
                }

                if (refresh_current_category)
                {
                    GAME::RefreshGames(false);
                }

                if (download_thumbnails)
                {
                    GAME::DownloadThumbnails(current_category);
                }
                
                if (uninstall_game)
                {
					handle_uninstall_game = true;
					game_uninstalled = 0;
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

                if (add_rom_game)
                    handle_add_rom_game = true;

                if (move_game)
                    handle_move_game = true;

                if (add_psp_iso_game)
                    handle_add_iso_game = true;

                if (add_eboot_game)
                    handle_add_eboot_game = true;
                
                if (add_folder)
                {
                    sprintf(temp_folder.title, "Folder");
                    sprintf(temp_folder.icon_path, "ux0:app/SMLA00001/folder.png");
                    categories_selection.clear();
                    for (int i=1; i<TOTAL_CATEGORY; i++)
                    {
                        CategorySelection cat;
                        sprintf(cat.category, "%s", game_categories[i].category);
                        sprintf(cat.name, "%s", game_categories[i].alt_title);
                        if (current_category->id == game_categories[i].id)
                        {
                            cat.selected = true;
                        }
                        else
                        {
                            cat.selected = false;
                        }
                        
                        categories_selection.push_back(cat);
                    }
                    handle_new_folder = true;
                }

                if (edit_folder)
                {
                    Folder *folder = GAME::FindFolder(current_category, selected_game->folder_id);
                    sprintf(temp_folder.category, folder->category);
                    sprintf(temp_folder.title, folder->title);
                    sprintf(temp_folder.icon_path, folder->icon_path);
                    temp_folder.id = folder->id;
                    handle_edit_delete_folder = true;
                }

                show_categories_as_tabs = show_categories_as_tabs_settings;
                show_all_categories = show_all_categories_setting;
                SetModalMode(false);
                move_game = false;
                remove_from_cache = false;
                refresh_current_category = false;
                add_psp_iso_game = false;
                add_eboot_game = false;
                add_rom_game = false;
                rename_game = false;
                download_thumbnails = false;
                uninstall_game = false;
                add_folder = false;
                edit_folder = false;
                
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleAdrenalineGame()
    {
        SetModalMode(true);
        GameCategory *category = categoryMap[game_to_boot->category];
        char popup_title[64];
        sprintf(popup_title, "Boot %s Game", category->alt_title);
        ImGui::OpenPopup(popup_title);
        if (category->id == PS1_GAMES && strcmp(category->rom_launcher_title_id, RETROARCH_TITLE_ID) == 0)
        {
            ImGui::SetNextWindowPos(ImVec2(300, 200));
            ImGui::SetNextWindowSize(ImVec2(400,100));
        }
        else
        {
            ImGui::SetNextWindowPos(ImVec2(230, 80));
            ImGui::SetNextWindowSize(ImVec2(495,385));
        }
        if (ImGui::BeginPopupModal(popup_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            float posX = ImGui::GetCursorPosX();
            if (category->id == PS1_GAMES)
            {
                ImGui::Text("Boot with: "); ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 110);
                if (ImGui::RadioButton("Adrenaline", strcmp(category->rom_launcher_title_id, DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID)==0))
                { 
                    sprintf(category->rom_launcher_title_id, "%s", DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("RetroArch", strcmp(category->rom_launcher_title_id, RETROARCH_TITLE_ID)==0))
                { 
                    sprintf(category->rom_launcher_title_id, "%s", RETROARCH_TITLE_ID);
                }
            }
            else
            {
                ImGui::Text("Boot Settings");
            }

            if (category->id != PS1_GAMES || strcmp(category->rom_launcher_title_id, RETROARCH_TITLE_ID) != 0)
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
                if (ImGui::RadioButton("266/133", settings.cpu_speed == CPU_266_133)) { settings.cpu_speed = CPU_266_133; } ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 320);
                if (ImGui::RadioButton("222/111", settings.cpu_speed == CPU_222_111)) { settings.cpu_speed = CPU_222_111; }
                ImGui::SetCursorPosX(posX + 110);
                if (ImGui::RadioButton("200/100", settings.cpu_speed == CPU_200_100)) { settings.cpu_speed = CPU_200_100; } ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 220);
                if (ImGui::RadioButton("166/83", settings.cpu_speed == CPU_166_83)) { settings.cpu_speed = CPU_166_83; } ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 320);
                if (ImGui::RadioButton("100/50", settings.cpu_speed == CPU_100_50)) { settings.cpu_speed = CPU_100_50; }
                ImGui::SetCursorPosX(posX + 110);
                if (ImGui::RadioButton("133/66", settings.cpu_speed == CPU_133_66)) { settings.cpu_speed = CPU_133_66; } ImGui::SameLine();
                ImGui::SetCursorPosX(posX + 220);
                if (ImGui::RadioButton("50/25", settings.cpu_speed == CPU_50_25)) { settings.cpu_speed = CPU_50_25; }
            }

            ImGui::Separator();
            if (ImGui::Button("OK"))
            {
                if (category->id == PS1_GAMES)
                {
                    OpenIniFile(CONFIG_INI_FILE);
                    WriteString(category->title, CONFIG_ROM_LAUNCHER_TITLE_ID, category->rom_launcher_title_id);
                    WriteIniFile(CONFIG_INI_FILE);
                    CloseIniFile();
                }
                DB::SavePspGameSettings(game_to_boot->rom_path, &settings);
                SetModalMode(false);
                handle_boot_game = false;
                GAME::Launch(game_to_boot, &settings);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                SetModalMode(false);
                settings = defaul_boot_settings;
                handle_boot_game = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    void HandleBootRomGame()
    {
        SetModalMode(true);
        GameCategory *cat = categoryMap[game_to_boot->category];

        if (retro_cores.size() == 0)
        {
            retro_cores.push_back(cat->core);
            for (int i=0; i<cat->alt_cores.size(); i++)
            retro_cores.push_back(cat->alt_cores[i]);
        }
        
        ImGui::OpenPopup("Select Retro Core");
        ImGui::SetNextWindowPos(ImVec2(260, 150));
        ImGui::SetNextWindowSize(ImVec2(400,230));
        if (ImGui::BeginPopupModal("Select Retro Core", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(390,160));
            ImGui::BeginChild("retrocore list");
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetWindowFocus();
            }

            for (int i = 0; i < retro_cores.size(); i++)
            {
                if (ImGui::Selectable(retro_cores[i].c_str()) &&
                    game_to_boot != nullptr)
                {
                    DB::SaveRomCoreSettings(game_to_boot->rom_path, retro_cores[i].c_str());
                    GAME::Launch(game_to_boot, nullptr, retro_cores[i].c_str());
                }
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                {
                    if (strcmp(retro_core, retro_cores[i].c_str()) == 0)
                    {
                        SetNavFocusHere();
                        sprintf(retro_core, "");
                    }
                }
                ImGui::Separator();
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button("Cancel"))
            {
                SetModalMode(false);
                handle_boot_rom_game = false;
                retro_cores.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleAddNewRomGame()
    {
        SetModalMode(true);
        static Game game;

        if (!game_added)
        {
            char game_type[64];
            sprintf(game_type, "Select %s game", current_category->alt_title);
            ImGui::OpenPopup(game_type);
            ImGui::SetNextWindowPos(ImVec2(150, 100));
            ImGui::SetNextWindowSize(ImVec2(630,330));
            if (ImGui::BeginPopupModal(game_type, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
            {
                if (games_on_filesystem.size() == 0)
                {
                    games_on_filesystem = FS::ListFiles(current_category->roms_path);
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
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(620,260));
                ImGui::BeginChild(game_type);
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
                            int dot_index = games_on_filesystem[i].find_last_of(".");
                            int slash_index = games_on_filesystem[i].find_last_of("/");
                            if (slash_index != std::string::npos)
                            {
                                strlcpy(game.title, games_on_filesystem[i].substr(slash_index+1, dot_index-slash_index-1).c_str(), 128);
                            }
                            else
                            {
                                strlcpy(game.title, games_on_filesystem[i].substr(0, dot_index).c_str(), 128);
                            }
                            if (current_category->id == MAME_2000_GAMES || current_category->id == MAME_2003_GAMES || current_category->id == NEOGEO_GAMES)
                            {
                                DB::GetMameRomName(nullptr, game.title, game.title);
                            }
                            game.tex = no_icon;

                            sprintf(game_action_message, "The game already exists in the cache.");
                            if (!DB::GameExists(nullptr, &game))
                            {
                                current_category->current_folder->games.push_back(game);
                                DB::InsertGame(nullptr, &game);
                                GAME::DownloadThumbnail(nullptr, &game);
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
                    SetModalMode(false);
                    handle_add_rom_game = false;
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
                    SetModalMode(false);
                    handle_add_rom_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
    }

    void HandleAddNewIsoGame()
    {
        SetModalMode(true);
        static Game game;

        if (!game_added)
        {
            ImGui::OpenPopup("Select PSP ISO game");
            ImGui::SetNextWindowPos(ImVec2(150, 100));
            ImGui::SetNextWindowSize(ImVec2(620,330));
            if (ImGui::BeginPopupModal("Select PSP ISO game", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
            {
                if (games_on_filesystem.size() == 0)
                {
                    games_on_filesystem = FS::ListFiles(pspemu_iso_path);
                    for (std::vector<std::string>::iterator it=games_on_filesystem.begin(); 
                        it!=games_on_filesystem.end(); )
                    {
                        int index = it->find_last_of(".");
                        if (index == std::string::npos || !GAME::IsRomExtension(it->substr(index), psp_iso_extensions))
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
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(610,260));
                ImGui::BeginChild("psp iso game list");
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetWindowFocus();
                }

                for (int i = 0; i < games_on_filesystem.size(); i++)
                {
                    if (ImGui::Selectable(games_on_filesystem[i].c_str()))
                    {
                        if (strlen(pspemu_iso_path) + games_on_filesystem[i].length() + 1 < 192)
                        {
                            sprintf(game.rom_path, "%s/%s", pspemu_iso_path, games_on_filesystem[i].c_str());
                            game.type = TYPE_PSP_ISO;
                            if (DB::GameExists(nullptr, &game))
                            {
                                sprintf(game_action_message, "The game already exists in the cache.");
                            }
                            else
                            {
                                sqlite3 *db;
                                sqlite3_open(CACHE_DB_FILE, &db);
                                try
                                {
                                    char title_id[12];
                                    DB::GetMaxTitleIdByType(db, TYPE_PSP_ISO,title_id);
                                    std::string str = std::string(title_id);
                                    int game_id = std::stoi(str.substr(5))+1;
                                    GAME::PopulateIsoGameInfo(&game, games_on_filesystem[i], game_id);
                                    categoryMap[game.category]->current_folder->games.push_back(game);
                                    DB::InsertGame(db, &game);
                                    GAME::SortGames(categoryMap[game.category]);
                                    GAME::SetMaxPage(categoryMap[game.category]);
                                    sprintf(game_action_message, "The game has being added to the cache.");
                                }
                                catch(const std::exception& e)
                                {
                                    sprintf(game_action_message, "Could not add game because it could be corrupted.");
                                }
                                sqlite3_close(db);
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
                    SetModalMode(false);
                    handle_add_iso_game = false;
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
                    SetModalMode(false);
                    handle_add_iso_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
    }

    void HandleAddNewEbootGame()
    {
        SetModalMode(true);
        static Game game;

        if (!game_added)
        {
            ImGui::OpenPopup("Select EBOOT game");
            ImGui::SetNextWindowPos(ImVec2(150, 100));
            ImGui::SetNextWindowSize(ImVec2(630,330));
            if (ImGui::BeginPopupModal("Select EBOOT game", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
            {
                if (games_on_filesystem.size() == 0)
                {
                    games_on_filesystem = FS::ListFiles(pspemu_eboot_path);
                    for (std::vector<std::string>::iterator it=games_on_filesystem.begin(); 
                        it!=games_on_filesystem.end(); )
                    {
                        int index = it->find_last_of(".");
                        if (index == std::string::npos || !GAME::IsRomExtension(it->substr(index), eboot_extensions))
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
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(620,260));
                ImGui::BeginChild("eboot game list");
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetWindowFocus();
                }

                for (int i = 0; i < games_on_filesystem.size(); i++)
                {
                    if (ImGui::Selectable(games_on_filesystem[i].c_str()))
                    {
                        if (strlen(pspemu_eboot_path) + games_on_filesystem[i].length() + 1 < 192)
                        {
                            sprintf(game.rom_path, "%s/%s", pspemu_eboot_path, games_on_filesystem[i].c_str());
                            game.type = TYPE_EBOOT;
                            if (DB::GameExists(nullptr, &game))
                            {
                                sprintf(game_action_message, "The game already exists in the cache.");
                            }
                            else
                            {
                                sqlite3 *db;
                                sqlite3_open(CACHE_DB_FILE, &db);
                                try
                                {
                                    char title_id[12];
                                    DB::GetMaxTitleIdByType(db, TYPE_EBOOT, title_id);
                                    std::string str = std::string(title_id);
                                    int game_id = std::stoi(str.substr(5))+1;
                                    GAME::PopulateEbootGameInfo(&game, games_on_filesystem[i], game_id);
                                    DB::InsertGame(db, &game);
                                    GAME::SortGames(categoryMap[game.category]);
                                    GAME::SetMaxPage(categoryMap[game.category]);
                                    sprintf(game_action_message, "The game has being added to the cache.");
                                }
                                catch(const std::exception& e)
                                {
                                    sprintf(game_action_message, "Could not add game because it could be corrupted.");
                                }
                                sqlite3_close(db);
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
                    SetModalMode(false);
                    handle_add_eboot_game = false;
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
                    SetModalMode(false);
                    handle_add_eboot_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
    }

    void MoveGameToCategory(sqlite3 *cache_db, sqlite3 *vita_db, Game *game, GameCategory *category)
    {
        GameCategory *current_category = categoryMap[game->category];
        if (game->type > TYPE_ROM && game->type < TYPE_SCUMMVM)
        {
            Game tmp = *game;
            sprintf(tmp.category, "%s", category->category);
            tmp.tex = no_icon;
            tmp.visible = false;
            tmp.thread_started = false;
            tmp.folder_id = 0;
            tmp.selected = false;
            category->folders[0].games.push_back(tmp);
            DB::UpdateGame(cache_db, &tmp);
            DB::UpdateFavoritesGameCategoryByRomPath(cache_db, &tmp);
            GAME::SortGames(category);
            GAME::SetMaxPage(category);
            GAME::RemoveGameFromCategory(current_category, game);
            GAME::SetMaxPage(current_category);
        } else if (selected_game->type == TYPE_BUBBLE)
        {
            category->valid_title_ids.push_back(game->id);
            CONFIG::RemoveFromMultiValues(current_category->valid_title_ids, selected_game->id);
            OpenIniFile(CONFIG_INI_FILE);
            WriteString(category->title, CONFIG_TITLE_ID_PREFIXES, CONFIG::GetMultiValueString(category->valid_title_ids).c_str());
            WriteString(current_category->title, CONFIG_TITLE_ID_PREFIXES, CONFIG::GetMultiValueString(current_category->valid_title_ids).c_str());
            WriteIniFile(CONFIG_INI_FILE);
            CloseIniFile();
            sprintf(game->category, "%s", category->category);
            DB::UpdateFavoritesGameCategoryById(cache_db, game);
            DB::DeleteVitaAppFolderById(vita_db, game->id);
            Game tmp = *game;
            tmp.tex = no_icon;
            tmp.visible = false;
            tmp.thread_started = false;
            tmp.folder_id = 0;
            tmp.selected = false;
            category->folders[0].games.push_back(tmp);
            GAME::SortGames(category);
            GAME::SetMaxPage(category);
            GAME::RemoveGameFromCategory(current_category, game);
            GAME::SetMaxPage(current_category);
        }
    }

    void MoveGameToFolder(sqlite3 *cache_db, sqlite3 *vita_db, Game *game, Folder *folder)
    {
        GameCategory *current_category = categoryMap[game->category];
        Game tmp = *game;
        tmp.tex = no_icon;
        tmp.visible = false;
        tmp.thread_started = false;
        tmp.folder_id = folder->id;
        tmp.selected = false;
        folder->games.push_back(tmp);

        if (game->type != TYPE_BUBBLE)
        {
            DB::UpdateGame(cache_db, &tmp);
        }
        else if (game->type == TYPE_BUBBLE)
        {
            int count = DB::UpdateVitaAppFolder(vita_db, tmp.id, folder->id);
            if (count < 1)
            {
                DB::InsertVitaAppFolder(vita_db, tmp.id, folder->id);
            }
        }
        
        GAME::RemoveGameFromFolder(current_category->current_folder, game);
    }

    void HandleMoveGame()
    {
        SetModalMode(true);
        if (!game_moved)
        {
            ImGui::OpenPopup("Select Move Location");
            ImGui::SetNextWindowPos(ImVec2(230, 100));
            ImGui::SetNextWindowSize(ImVec2(490,360));
            if (ImGui::BeginPopupModal("Select Move Location", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,260));
                ImGui::Text("Move To:"); ImGui::SameLine();
                ImGui::RadioButton("Folder", &move_location, 0); ImGui::SameLine();
                ImGui::RadioButton("Category", &move_location, 1);
                ImGui::Separator();

                if (move_location == 1)
                {
                    ImGui::BeginChild("category list");

                    for (int i = 1; i < TOTAL_CATEGORY-1; i++)
                    {
                        if (current_category->id != game_categories[i].id)
                        {
                            if (ImGui::Selectable(game_categories[i].alt_title) &&
                                selected_game != nullptr)
                            {
                                if (!selection_mode)
                                {
                                    if ((selected_game->type == TYPE_ROM || selected_game->type == TYPE_SCUMMVM || selected_game->type == TYPE_FOLDER))
                                    {
                                        sprintf(game_action_message, "Can't move ROM type games. Since they\nare dependent on RetroArch core of \nthe category.");
                                    }
                                    else
                                    {
                                        MoveGameToCategory(nullptr, nullptr, selected_game, &game_categories[i]);
                                        sprintf(game_action_message, "Game moved to %s category", game_categories[i].alt_title);
                                    }
                                    
                                }
                                else
                                {
                                    sqlite3 *cache_db, *vita_db;
                                    sqlite3_open(CACHE_DB_FILE, &cache_db);
                                    sqlite3_open(VITA_APP_DB_FILE, &vita_db);
                                    std::vector<Game> list = GAME::GetSelectedGames(current_category);
                                    int games_not_moved = 0;
                                    for (int j=0; j<list.size(); j++)
                                    {
                                        Game *game = &list[j];
                                        if (game->type == TYPE_BUBBLE || game->type == TYPE_EBOOT || game->type == TYPE_PSP_ISO)
                                        {
                                            MoveGameToCategory(cache_db, vita_db, game, &game_categories[i]);
                                        }
                                        else
                                        {
                                            games_not_moved++;
                                        }
                                        
                                    }
                                    sqlite3_close(cache_db);
                                    sqlite3_close(vita_db);
                                    if (games_not_moved == 0)
                                    {
                                        sprintf(game_action_message, "All selected games moved to %s category", game_categories[i].alt_title);
                                    }
                                    else
                                    {
                                        sprintf(game_action_message, "Some selected games wasn't moved to %s category because they ROM games", game_categories[i].alt_title);
                                    }
                                    
                                    GAME::ClearSelection(current_category);
                                    selection_mode = false;
                                }
                                
                                game_moved = true;
                            }
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                }
                else if (move_location == 0)
                {
                    ImGui::BeginChild("folder list");

                    for (int i = 0; i <current_category->folders.size(); i++)
                    {
                        if (current_category->current_folder->id != current_category->folders[i].id)
                        {
                            if (ImGui::Selectable(current_category->folders[i].title) &&
                                selected_game != nullptr)
                            {
                                if (!selection_mode)
                                {
                                    MoveGameToFolder(nullptr, nullptr, selected_game, &current_category->folders[i]);
                                    GAME::SortGames(&current_category->folders[i]);
                                    GAME::SetMaxPage(current_category);
                                    sprintf(game_action_message, "Game moved to %s folder", current_category->folders[i].title);
                                }
                                else
                                {
                                    sqlite3 *cache_db, *vita_db;
                                    sqlite3_open(CACHE_DB_FILE, &cache_db);
                                    sqlite3_open(VITA_APP_DB_FILE, &vita_db);

                                    std::vector<Game> list = GAME::GetSelectedGames(current_category);
                                    for (int j=0; j<list.size(); j++)
                                    {
                                        Game *game = &list[j];
                                        MoveGameToFolder(cache_db, vita_db, game, &current_category->folders[i]);
                                    }
                                    sqlite3_close(cache_db);
                                    sqlite3_close(vita_db);

                                    GAME::SortGames(&current_category->folders[i]);
                                    GAME::SetMaxPage(current_category);
                                    sprintf(game_action_message, "Games moved to %s folder", current_category->folders[i].title);
                                    GAME::ClearSelection(current_category);
                                    selection_mode = false;
                                }
                                game_moved = true;
                            }
                            ImGui::Separator();
                        }
                    }
                    ImGui::EndChild();
                }

                ImGui::Separator();
                if (ImGui::Button("Cancel"))
                {
                    SetModalMode(false);
                    handle_move_game = false;
                    game_moved = false;
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
                    game_moved = false;
                    SetModalMode(false);
                    handle_move_game = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
    }

    void HandleAddNewFolder()
    {
        SetModalMode(true);
        ImGui::OpenPopup("Create New Folder");
        ImGui::SetNextWindowPos(ImVec2(230, 100));
        ImGui::SetNextWindowSize(ImVec2(490,340));
        if (ImGui::BeginPopupModal("Create New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,260));

            ImGui::Text("Name:"); ImGui::SameLine();
            if (ImGui::Selectable(temp_folder.title, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(400, 0)))
            {
                ime_single_field = temp_folder.title;
                ime_before_update = nullptr;
                ime_after_update = nullptr;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog("Folder Name", temp_folder.title, 127, SCE_IME_TYPE_DEFAULT, 0, 0);
                gui_mode = GUI_MODE_IME;
            };
            ImGui::Separator();
            
            ImGui::Text("Icon Path:"); ImGui::SameLine();
            if (ImGui::Selectable(temp_folder.icon_path, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(370, 0)))
            {
                ime_single_field = temp_folder.icon_path;
                ime_before_update = nullptr;
                ime_after_update = nullptr;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog("Icon Path", temp_folder.icon_path, 127, SCE_IME_TYPE_DEFAULT, 0, 0);
                gui_mode = GUI_MODE_IME;
            };
            ImGui::Separator();
            
            ImGui::Text("Categories:");
            ImGui::ListBoxHeader("##categories", ImVec2(470, 184));
            for (int i=0; i<categories_selection.size(); i++)
            {
                ImGui::Checkbox(categories_selection[i].name, &categories_selection[i].selected);
            }
            ImGui::ListBoxFooter();
            ImGui::Separator();

            if (ImGui::Button("OK"))
            {
                sqlite3 *db;
                sqlite3_open(CACHE_DB_FILE, &db);
                for (int i=0; i<categories_selection.size(); i++)
                {
                    if (categories_selection[i].selected)
                    {
                        GameCategory *category = categoryMap[categories_selection[i].category];
                        Folder folder;
                        sprintf(folder.category, "%s", categories_selection[i].category);
                        sprintf(folder.title, "%s", temp_folder.title);
                        sprintf(folder.icon_path, "%s", temp_folder.icon_path);
                        DB::InsertFolder(db, &folder);
                        int current_folder_id = category->current_folder->id;
                        category->folders.push_back(folder);
                        category->current_folder = GAME::FindFolder(category, current_folder_id);

                        Game game;
                        sprintf(game.id, "%d", folder.id);
                        sprintf(game.title, "%s", folder.title);
                        sprintf(game.category, "%s", folder.category);
                        game.type = TYPE_FOLDER;
                        game.folder_id = folder.id;
                        game.favorite = false;
                        game.tex = no_icon;
                        Folder *cat_folder = GAME::FindFolder(category, folder.id);
                        category->folders[0].games.insert(category->folders[0].games.begin(), game);
                        GAME::SortGames(&category->folders[0]);
                    }
                }
                sqlite3_close(db);
                SetModalMode(false);
                handle_new_folder = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                SetModalMode(false);
                handle_new_folder = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void HandleEditDeleteFolder()
    {
        SetModalMode(true);
        ImGui::OpenPopup("Edit/Delete Folder");
        ImGui::SetNextWindowPos(ImVec2(230, 200));
        ImGui::SetNextWindowSize(ImVec2(490,140));
        if (ImGui::BeginPopupModal("Edit/Delete Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(480,260));

            ImGui::Text("Name:"); ImGui::SameLine();
            if (ImGui::Selectable(temp_folder.title, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(400, 0)))
            {
                ime_single_field = temp_folder.title;
                ime_before_update = nullptr;
                ime_after_update = nullptr;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog("Folder Name", temp_folder.title, 127, SCE_IME_TYPE_DEFAULT, 0, 0);
                gui_mode = GUI_MODE_IME;
            };
            ImGui::Separator();
            
            ImGui::Text("Icon Path:"); ImGui::SameLine();
            if (ImGui::Selectable(temp_folder.icon_path, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(370, 0)))
            {
                ime_single_field = temp_folder.icon_path;
                ime_before_update = nullptr;
                ime_after_update = nullptr;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog("Icon Path", temp_folder.icon_path, 127, SCE_IME_TYPE_DEFAULT, 0, 0);
                gui_mode = GUI_MODE_IME;
            };
            ImGui::Separator();
            
            if (ImGui::Button("Save"))
            {
                Folder *folder = GAME::FindFolder(current_category, temp_folder.id);
                sprintf(folder->title, temp_folder.title);
                sprintf(folder->icon_path, temp_folder.icon_path);
                DB::UpdateFolder(nullptr, folder);
                for (int i=0; i<current_category->folders[0].games.size(); i++)
                {
                    if (current_category->folders[0].games[i].folder_id == folder->id)
                    {
                        sprintf(current_category->folders[0].games[i].title, "%s", folder->title);
                        break;
                    }
                }
                SetModalMode(false);
                handle_edit_delete_folder = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                SetModalMode(false);
                handle_edit_delete_folder = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Delete"))
            {
                GAME::MoveGamesBetweenFolders(current_category, temp_folder.id, FOLDER_ROOT_ID);
                DB::DeleteFolder(nullptr, &temp_folder);
                DB::DeleteVitaAppFolder(nullptr, temp_folder.id);
                GAME::RemoveFolderFromCategory(current_category, temp_folder.id);
                Game game;
                game.folder_id = temp_folder.id;
                game.type = TYPE_FOLDER;
                GAME::RemoveGameFromCategory(current_category, &game);
                GAME::SortGames(current_category);
                GAME::SetMaxPage(current_category);
                SetModalMode(false);
                handle_edit_delete_folder = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void HandleSearchGame()
    {
        SetModalMode(true);
        static Game *search_selected_game = nullptr;
        SceCtrlData pad;
        sceCtrlPeekBufferNegative(0, &pad, 1);

        if ((pad_prev.buttons & SCE_CTRL_SQUARE) && !(pad.buttons & SCE_CTRL_SQUARE))
        {
            if (search_selected_game != nullptr && search_selected_game->type != TYPE_FOLDER)
            {
                if (!search_selected_game->favorite)
                {
                    Game game = *search_selected_game;
                    game.tex = no_icon;
                    game.visible = false;
                    game.thread_started = false;
                    game_categories[FAVORITES].current_folder->games.push_back(game);
                    GAME::SortGames(&game_categories[FAVORITES]);
                    GAME::SetMaxPage(&game_categories[FAVORITES]);
                    search_selected_game->favorite = true;
                    Game *cat_game = GAME::FindGame(categoryMap[search_selected_game->category], search_selected_game);
                    cat_game->favorite = true;
                    DB::InsertFavorite(nullptr, search_selected_game);
                }
                else {
                    search_selected_game->favorite = false;
                    DB::DeleteFavorite(nullptr, search_selected_game);
                    GAME::RemoveGameFromCategory(&game_categories[FAVORITES], search_selected_game);
                    GAME::SetMaxPage(&game_categories[FAVORITES]);
                }
            }
        }

        ImGui::OpenPopup("Search Games");
        ImGui::SetNextWindowPos(ImVec2(180, 100));
        ImGui::SetNextWindowSize(ImVec2(600,357));
        if (ImGui::BeginPopupModal("Search Games", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::Text("Category:"); ImGui::SameLine();
            if (ImGui::BeginCombo("##Category", cb_category_name, ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightRegular))
            {
                bool is_selected = strcmp("all", cb_category_name)==0;
                if (ImGui::Selectable("all", is_selected))
                {
                    sprintf(cb_category_name, "all");
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                for (int n = 1; n < TOTAL_CATEGORY; n++)
                {
                    const bool is_selected = strcmp(game_categories[n].category, cb_category_name)==0;
                    if (ImGui::Selectable(game_categories[n].alt_title, is_selected))
                        sprintf(cb_category_name, "%s", game_categories[n].category);

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();

            ImGui::Text("Search:"); ImGui::SameLine();
            if (ImGui::Selectable(txt_search_text, false, ImGuiSelectableFlags_DontClosePopups, ImVec2(300, 0)))
            {
                ime_single_field = txt_search_text;
                ime_before_update = nullptr;
                ime_after_update = nullptr;
                ime_callback = SingleValueImeCallback;
                Dialog::initImeDialog("Search For", txt_search_text, 32, SCE_IME_TYPE_DEFAULT, 0, 0);
                gui_mode = GUI_MODE_IME;
            };
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::SmallButton("Find"))
            {
                if (strlen(txt_search_text) < 3)
                {
                    sprintf(txt_search_text, "min 3 character required");
                }
                else
                {
                    OpenIniFile(CONFIG_INI_FILE);
                    WriteString(CONFIG_GLOBAL, CONFIG_SEARCH_TEXT, txt_search_text);
                    WriteIniFile(CONFIG_INI_FILE);
                    CloseIniFile();
                    std::vector<GameCategory*> cats;
                    if (strcmp("all", cb_category_name) == 0)
                    {
                        for (int i=1; i<TOTAL_CATEGORY; i++)
                        {
                            cats.push_back(&game_categories[i]);
                        }
                    }
                    else
                    {
                        cats.push_back(categoryMap[cb_category_name]);
                    }
                    
                    games_selection.clear();
                    GAME::FindGamesByPartialName(cats, txt_search_text, games_selection);
                }
                
            }
            ImGui::Separator();
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(595,238));
            ImGui::BeginChild("search games list");
            ImGui::Columns(2, "search games list", true);
            for (int i = 0; i < games_selection.size(); i++)
            {
                ImGui::SetColumnWidth(-1,450);
                if (games_selection[i].favorite)
                {
                    ImGui::Image(reinterpret_cast<ImTextureID>(favorite_icon.id), ImVec2(16,16));
                    ImGui::SameLine();
                }
                char title[192];
                sprintf(title, "%s##%s%d%d", games_selection[i].title, games_selection[i].category, search_count, i);
                if (ImGui::Selectable(title, false, ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_SpanAllColumns))
                {
                    Game *game = &games_selection[i];
                    if (game->type == TYPE_BUBBLE || game->type == TYPE_SCUMMVM)
                    {
                        GAME::Launch(game);
                    }
                    else if (game->type == TYPE_ROM)
                    {
                        GameCategory *cat = categoryMap[game->category];
                        if (cat->alt_cores.size() == 0 || !cat->boot_with_alt_core)
                        {
                            GAME::Launch(game);
                        }
                        handle_boot_rom_game = true;
                        game_to_boot = game;
                        sprintf(retro_core, "%s", cat->core);
                        DB::GetRomCoreSettings(game_to_boot->rom_path, retro_core);
                    }
                    else
                    {
                        handle_boot_game = true;
                        game_to_boot = game;
                        settings = defaul_boot_settings;
                        DB::GetPspGameSettings(game_to_boot->rom_path, &settings);
                    }
                    SetModalMode(false);
                    handle_search_game = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    search_selected_game = &games_selection[i];
                }
                ImGui::NextColumn();
                ImGui::Text(categoryMap[games_selection[i].category]->alt_title);
                ImGui::NextColumn();
                ImGui::Separator();
            }
            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button("Cancel"))
            {
                SetModalMode(false);
                handle_search_game = false;
                games_selection.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

	void HandleUninstallGame()
	{
        SetModalMode(true);
        if (game_uninstalled == 0)
        {
			GAME::StartUninstallGameThread(selected_game);
			game_uninstalled = 1;
		}
		
		if (game_uninstalled == 0 || game_uninstalled == 1)
		{
			sprintf(game_action_message, "Please wait while application is uninstalled");
		}
		else if (game_uninstalled == 2)
		{
			sprintf(game_action_message, "Application successfully uninstalled");
		}
		else
		{
			sprintf(game_action_message, "Error encounter uninstalling application");
		}
		
		ImGui::OpenPopup("Info");
		ImGui::SetNextWindowPos(ImVec2(250, 220));
		if (ImGui::BeginPopupModal("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushTextWrapPos(400);
			ImGui::Text("%s", game_action_message);
			ImGui::PopTextWrapPos();

			if (game_uninstalled > 1 )
			{
				ImGui::Separator();
				if (ImGui::Button("OK"))
				{
					game_uninstalled = -1;
					SetModalMode(false);
					handle_uninstall_game = false;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
    }
	
    void GameScanWindow()
    {
        Windows::SetupWindow();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);

        if (ImGui::Begin("Game Launcher", nullptr, ImGuiWindowFlags_NoDecoration)) {
            static float progress = 0.0f;
            if (current_category->current_folder->games.size() > 0 && games_to_scan > 0)
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
        sprintf(game_categories[CATEGORY].current_folder->games[tmp_category->id].title, tmp_category->alt_title);
        SetModalMode(false);
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
