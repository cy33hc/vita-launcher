#include <cstdint>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include <vitasdk.h>

#include "game.h"
#include "sfo.h"
#include "fs.h"
#include "windows.h"
#include "textures.h"
#include "db.h"

#define FAVORITES_FILE "ux0:data/SMLA00001/favorites.txt"
#define NUM_CACHED_PAGES 5

GameCategory game_categories[TOTAL_CATEGORY];
std::map<std::string, GameCategory*> categoryMap;

GameCategory *current_category;

bool game_scan_complete = false;
int games_to_scan = 1;
int games_scanned = 0;
Game game_scan_inprogress;
char scan_message[256];
int ROM_CATEGORIES[TOTAL_ROM_CATEGORY] = {PS1_GAMES, NES_GAMES, SNES_GAMES, GB_GAMES, GBA_GAMES, N64_GAMES, GBC_GAMES, NEC_GAMES,
                         GBC_GAMES, NEOGEO_GAMES, GAME_GEAR_GAMES, MASTER_SYSTEM_GAMES, MEGA_DRIVE_GAMES};

bool use_game_db = true;

namespace GAME {

    void Init() {
    }

    void Scan()
    {
        current_category = &game_categories[VITA_GAMES];

        sprintf(scan_message, "%s", "Reading game info from vita app database");
        games_to_scan = DB::GetVitaDbGamesCount();
        games_scanned = 0;
        DB::GetVitaDbGames(current_category);
        
        if (!FS::FileExists(CACHE_DB_FILE))
        {
            sqlite3 *db;
            sqlite3_open(CACHE_DB_FILE, &db);
            DB::SetupDatabase(db);

            for (int i=0; i<TOTAL_ROM_CATEGORY; i++)
            {
                int category_id = ROM_CATEGORIES[i];

                GameCategory *category = &game_categories[category_id];
                std::vector<std::string> files = FS::ListDir(category->roms_path);
                games_to_scan = files.size();
                games_scanned = 0;
                sprintf(scan_message, "Scanning for %s games in the %s folder", category->title, category->roms_path);

                for(std::size_t j = 0; j < files.size(); ++j) {
                    int index = files[j].find_last_of(".");
                    if (files[j].substr(index) != ".png")
                    {
                        Game game;
                        game.type = TYPE_ROM;
                        sprintf(game.id, "%s", category->title);
                        sprintf(game.category, "%s", category->category);
                        sprintf(game.rom_path, "%s/%s", category->roms_path, files[j].c_str());
                        sprintf(game.title, "%s", files[j].substr(0, index).c_str());
                        game.tex = no_icon;
                        current_category->games.push_back(game);
                        DB::InsertGame(db, &game);
                        game_scan_inprogress = game;
                        games_scanned++;
                    }
                    else
                    {
                        games_to_scan--;
                    }
                    
                }
            }
            sqlite3_close(db);
        }
        else {
            LoadGamesCache();
        }

        DB::GetFavorites(nullptr, &game_categories[FAVORITES]);

        for (std::vector<Game>::iterator it=game_categories[FAVORITES].games.begin(); 
             it!=game_categories[FAVORITES].games.end(); )
        {
            Game* game = FindGame(current_category, &*it);
            if (game != nullptr)
            {
                game->favorite = true;
                ++it;
            }
            else
            {
                it = game_categories[FAVORITES].games.erase(it);
                DB::DeleteFavorite(nullptr, &*it);
            }
        }
        
        
        for (std::vector<Game>::iterator it=current_category->games.begin();
            it!=current_category->games.end(); )
        {
            bool removed = false;
            for (int i=2; i<TOTAL_CATEGORY; i++)
            {
                GameCategory *category = &game_categories[i];
                if (strcmp(it->category, category->category) == 0)
                {
                    category->games.push_back(*it);
                    it = current_category->games.erase(it);
                    games_to_scan--;
                    removed = true;
                    break;
                }
            }

            if (!removed)
            {
                ++it;
            }
        }
        
        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            SortGames(&game_categories[i]);
            game_categories[i].page_num = 1;
            SetMaxPage(&game_categories[i]);
        }
    }

    void SetMaxPage(GameCategory *category)
    {
        category->max_page = (category->games.size() + 18 - 1) / 18;
        if (category->max_page == 0)
        {
            category->max_page = 1;
        }
    }

    bool Launch(Game *game) {
        if (game->type == TYPE_BUBBLE)
        {
            char uri[32];
            sprintf(uri, "psgm:play?titleid=%s", game->id);
            sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            sceKernelExitProcess(0);
        }
        else
        {
            GameCategory* category = GetRomCategoryByName(game->category);
            if (category != nullptr)
            {
                if (strcmp(category->rom_launcher_title_id, "RETROVITA") == 0)
                {
                    char uri[512];
                    sprintf(uri, "psgm:play?titleid=%s&param=%s&param2=%s", category->rom_launcher_title_id, category->core, game->rom_path);
                    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
                    sceKernelDelayThread(1000);
                    sceKernelExitProcess(0);
                }
                else if (strcmp(category->rom_launcher_title_id, "DEDALOX64") == 0)
                {
                    char uri[512];
                    sprintf(uri, "psgm:play?titleid=%s&param=%s", category->rom_launcher_title_id, game->rom_path);
                    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
                    sceKernelDelayThread(1000);
                    sceKernelExitProcess(0);
                }
            }
        }
        
    };

    std::string nextToken(std::vector<char> &buffer, int &nextTokenPos)
    {
        std::string token = "";
        if (nextTokenPos >= buffer.size())
        {
            return NULL;
        }

        for (int i = nextTokenPos; i < buffer.size(); i++)
        {
            if (buffer[i] == '|')
            {
                if ((i+1 < buffer.size()) && (buffer[i+1] == '|'))
                {
                    nextTokenPos = i+2;
                    return token;
                }
                else {
                    token += buffer[i];
                }
            }
            else if (buffer[i] == '\n') {
                nextTokenPos = i+1;
                return token;
            }
            else {
                token += buffer[i];
            }
        }

        return token;
    }

    void LoadGamesCache() {
        sqlite3 *db;
        sqlite3_open(CACHE_DB_FILE, &db);
        games_to_scan = DB::GetCachedGamesCount(db);
        games_scanned = 0;
        sprintf(scan_message, "%s", "Loading game info from cache");
        DB::GetCachedGames(db, current_category);
        sqlite3_close(db);
    };

    void LoadGameImages(int category, int prev_page, int page) {
        int high = 0;
        int low = 0;

        if (game_categories[category].max_page > NUM_CACHED_PAGES + 5)
        {
            int del_page = 0;

            if ((page > prev_page) or (prev_page == game_categories[category].max_page && page == 1))
            {
                del_page = DecrementPage(page, NUM_CACHED_PAGES);
            } else if ((page < prev_page) or (prev_page == 1 && page == game_categories[category].max_page))
            {
                del_page = IncrementPage(page, NUM_CACHED_PAGES);
            }

            int high = del_page * 18;
            int low = high - 18;
            if (del_page > 0)
            {
                for (int i=low; (i<high && i < game_categories[category].games.size()); i++)
                {
                    Game *game = &game_categories[category].games[i];
                    if (game->tex.id != no_icon.id)
                    {
                        Textures::Free(&game->tex);
                        game->tex = no_icon;
                    }
                }
            }
        }

        high = page * 18;
        low = high - 18;
        for(std::size_t i = low; (i < high && i < game_categories[category].games.size()); i++) {
            Game *game = &game_categories[category].games[i];
            if (page == current_category->page_num && category == current_category->id)
            {
                if (game->tex.id == no_icon.id)
                {
                    LoadGameImage(game);
                }
            }
            else
            {
                // No need to continue if page isn't in view
                return;
            }
            
        }
    }

    int IncrementPage(int page, int num_of_pages)
    {
        int new_page = page + num_of_pages;
        if (new_page > current_category->max_page)
        {
            new_page = new_page % current_category->max_page;
        }
        return new_page;
    }

    int DecrementPage(int page, int num_of_pages)
    {
        int new_page = page - num_of_pages;
        if (new_page > 0)
        {
            return new_page;
        }
        return new_page + current_category->max_page;
    }

    void LoadGameImage(Game *game) {
        Tex tex;
        tex = no_icon;

        char icon_path[256];
        if (game->type == TYPE_BUBBLE)
        {
            sprintf(icon_path, "ur0:appmeta/%s/icon0.png", game->id);
        }
        else
        {
            GameCategory* category = categoryMap[game->category];
            sprintf(icon_path, "%s/%s\.png", category->icon_path, game->title);
        }
        
        if (FS::FileExists(icon_path))
        {
            if (Textures::LoadImageFile(icon_path, &tex))
            {
                game->tex = tex;
            }
        }
    }

    void Exit() {
    }

	void StartLoadImagesThread(int category, int prev_page_num, int page)
	{
		LoadImagesParams page_param;
        page_param.category = category;
		page_param.prev_page_num = prev_page_num;
		page_param.page_num = page;
		load_images_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::LoadImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (load_images_thid >= 0)
			sceKernelStartThread(load_images_thid, sizeof(LoadImagesParams), &page_param);
	}

    int LoadImagesThread(SceSize args, LoadImagesParams *params) {
        sceKernelDelayThread(300000);
        GAME::LoadGameImages(params->category, params->prev_page_num, params->page_num);
        return sceKernelExitDeleteThread(0);
    }

    void StartScanGamesThread()
    {
        scan_games_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::ScanGamesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (scan_games_thid >= 0)
			sceKernelStartThread(scan_games_thid, 0, NULL);
    }

    int ScanGamesThread(SceSize args, void *argp)
    {
        game_scan_complete = false;
        sceKernelDelayThread(500000);
        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            game_categories[i].games.clear();
        }

        GAME::Scan();
        game_scan_complete = true;

        if (game_categories[FAVORITES].games.size() > 0)
        {
            current_category = &game_categories[FAVORITES];
        }
        current_category->page_num = 1;
        view_mode = current_category->view_mode;
        GAME::StartLoadImagesThread(current_category->id, 1, 1);
        return sceKernelExitDeleteThread(0);
    }

    int GameComparator(const void *v1, const void *v2)
    {
        const Game *p1 = (Game *)v1;
        const Game *p2 = (Game *)v2;
        int p1_len = strlen(p1->title);
        int p2_len = strlen(p2->title);
        int len = p1_len;
        if (p2_len < p1_len)
            len = p2_len;
        return strncmp(p1->title, p2->title, len);
    }

    void DeleteGamesImages(GameCategory *category)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            Game *game = &category->games[i];
            if (game->tex.id != no_icon.id)
            {
                Textures::Free(&game->tex);
                game->tex = no_icon;
            }
        }
    }

    Game* FindGame(GameCategory *category, Game *game)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            if ((game->type == TYPE_BUBBLE && strcmp(game->id, category->games[i].id) == 0) ||
                (game->type == TYPE_ROM && strcmp(game->rom_path, category->games[i].rom_path) == 0))
            {
                return &category->games[i];
            }
        }
        return nullptr;
    }

    int FindGamePosition(GameCategory *category, Game *game)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            if ((game->type == TYPE_BUBBLE && strcmp(game->id, category->games[i].id) == 0) ||
                (game->type == TYPE_ROM && strcmp(game->rom_path, category->games[i].rom_path) == 0))
            {
                return i;
            }
        }
        return -1;
    }

    int RemoveGameFromCategory(GameCategory *category, Game *game)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            if ((game->type == TYPE_BUBBLE && strcmp(game->id, category->games[i].id) == 0) ||
                (game->type == TYPE_ROM && strcmp(game->rom_path, category->games[i].rom_path) == 0))
            {
                category->games.erase(category->games.begin()+i);
                return i;
            }
        }
        return -1;
    }

    void SortGames(GameCategory *category)
    {
        qsort(&category->games[0], category->games.size(), sizeof(Game), GameComparator);
    }

    void RefreshGames()
    {
        
        FS::Rm(CACHE_DB_FILE);
        StartScanGamesThread();
        
    }

    bool IsMatchPrefixes(const char* id, std::vector<std::string> &prefixes)
    {
        for (int i=0; i < prefixes.size(); i++)
        {
            if (strncmp(id, prefixes[i].c_str(), strlen(prefixes[i].c_str())) == 0)
                return true;
        }
        return false;
    }

    const char* GetGameCategory(const char *title_id)
    {
        for (int i=1; i<TOTAL_CATEGORY; i++)
        {
            if (IsMatchPrefixes(title_id, game_categories[i].valid_title_ids) &&
                strncmp(title_id, "PSPEMUCFW", 9) != 0)
            {
                return game_categories[i].category;
            }
        }

        return game_categories[HOMEBREWS].category;
    }

    GameCategory* GetRomCategoryByName(const char* category_name)
    {
        for (int i=0; i<TOTAL_ROM_CATEGORY; i++)
        {
            int cat = ROM_CATEGORIES[i];
            if (strcmp(category_name, game_categories[cat].category) == 0)
            {
                return &game_categories[cat];
            }
        }
    }

    bool IsRomCategory(int categoryId)
    {
        for (int i=0; i<TOTAL_ROM_CATEGORY; i++)
        {
            if (categoryId == ROM_CATEGORIES[i])
            {
                return true;
            }
        }
        return false;
    }
}
