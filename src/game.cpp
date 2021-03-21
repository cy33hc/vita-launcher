#include <cstdint>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include <vitasdk.h>
#include <cstring>

#include "game.h"
#include "sfo.h"
#include "fs.h"
#include "gui.h"
#include "windows.h"
#include "textures.h"
#include "config.h"
#include "db.h"
#include "eboot.h"
#include "iso.h"
#include "cso.h"
#include "net.h"

#include "debugnet.h"
extern "C" {
	#include "inifile.h"
}

#define NUM_CACHED_PAGES 5

GameCategory game_categories[TOTAL_CATEGORY+1];
std::map<std::string, GameCategory*> categoryMap;
std::vector<std::string> psp_iso_extensions;
std::vector<std::string> eboot_extensions;
std::vector<std::string> hidden_title_ids;
char pspemu_path[16];
char pspemu_iso_path[32];
char pspemu_eboot_path[32];
char game_uninstalled = 0;

GameCategory *current_category;
std::vector<Game*> selected_games;

int games_to_scan = 1;
int games_scanned = 0;
Game game_scan_inprogress;
char scan_message[256];
int ROM_CATEGORIES[TOTAL_ROM_CATEGORY] = {PS1_GAMES, NES_GAMES, SNES_GAMES, GB_GAMES, GBA_GAMES, N64_GAMES, GBC_GAMES, NEC_GAMES,
                         NEOGEO_GAMES, NEOGEO_CD_GAMES, NEOGEO_PC_GAMES, GAME_GEAR_GAMES, MASTER_SYSTEM_GAMES, MEGA_DRIVE_GAMES,
                         SEGA_32X_GAMES, SEGA_CD_GAMES, SEGA_SATURN_GAMES, SEGA_DREAMCAST_GAMES, ATARI_2600_GAMES, ATARI_5200_GAMES,
                         ATARI_7800_GAMES, ATARI_LYNX_GAMES, AMIGA_GAMES, BANDAI_GAMES, C64_GAMES, MSX1_GAMES, MSX2_GAMES,
                         T_GRAFX_GAMES, VECTREX_GAMES, GAW_GAMES, MAME_2000_GAMES, MAME_2003_GAMES};

char adernaline_launcher_boot_bin_path[32];
char adernaline_launcher_title_id[12];
BootSettings defaul_boot_settings;

bool use_game_db = true;

namespace GAME {

    void Init() {
    }

    void Scan()
    {
        current_category = &game_categories[VITA_GAMES];

        if (!FS::FileExists(PER_GAME_SETTINGS_DB_FILE))
        {
            DB::SetupPerGameSettingsDatabase();
        }

        if (!FS::FileExists(CACHE_DB_FILE))
        {
            sprintf(scan_message, "%s", "Reading game info from vita app database");
            games_to_scan = DB::GetVitaDbGamesCount();
            games_scanned = 0;
            DB::GetVitaDbGames();
            
            sqlite3 *db;
            sqlite3_open(CACHE_DB_FILE, &db);
            DB::SetupDatabase(db);
            ScanRetroGames(db);
            ScanAdrenalineIsoGames(db);
            ScanAdrenalineEbootGames(db);
            ScanScummVMGames(db);
            sqlite3_close(db);
        }
        else {
            sqlite3 *db;
            sqlite3_open(CACHE_DB_FILE, &db);
            DB::UpdateDatabase(db);
            for (int i=0; i < TOTAL_CATEGORY; i++)
            {
                DB::GetFolders(db, &game_categories[i]);
            }

            sprintf(scan_message, "%s", "Reading game info from vita app database");
            games_to_scan = DB::GetVitaDbGamesCount();
            games_scanned = 0;
            DB::GetVitaDbGames();
            
            LoadGamesCache(db);
            sqlite3_close(db);
        }

        DB::GetFavorites(nullptr, &game_categories[FAVORITES]);

        for (std::vector<Game>::iterator it=game_categories[FAVORITES].current_folder->games.begin(); 
             it!=game_categories[FAVORITES].current_folder->games.end(); )
        {
            GameCategory *category = categoryMap[it->category];
            Game* game = FindGame(category, &*it);
            if (game != nullptr)
            {
                game->favorite = true;
            }
            ++it;
        }

        for (int i = 0; i < TOTAL_CATEGORY; i++)
        {

        }

        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            Game game;
            sprintf(game.id, "%d", game_categories[i].id);
            sprintf(game.title, "%s", game_categories[i].alt_title);
            sprintf(game.category, "%s", game_categories[i].category);
            game.type = TYPE_CATEGORY;
            game.folder_id = 0;
            game.favorite = false;
            game.tex = no_icon;
            game_categories[TOTAL_CATEGORY].current_folder->games.push_back(game);

            SortGames(&game_categories[i]);
            game_categories[i].current_folder->page_num = 1;
            SetMaxPage(&game_categories[i]);
        }
        GAME::SetMaxPage(&game_categories[TOTAL_CATEGORY]);
    }

    std::string str_tolower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), 
                [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    bool IsRomExtension(std::string str, std::vector<std::string> &file_filters)
    {
        if (std::find(file_filters.begin(), file_filters.end(), str_tolower(str)) != file_filters.end()) {
            return true;
        } else {
            return false;
        }
    }

    void PopulateIsoGameInfo(Game *game, std::string rom, int game_index)
    {
        int dot_index = rom.find_last_of(".");
        sprintf(game->id, "%s%04d", "SMLAP", game_index);
        char sfo_path[192];
        char icon_path[192];
        sprintf(sfo_path, "ux0:data/SMLA00001/data/%s", game->id);
        FS::MkDirs(sfo_path);
        sprintf(game->rom_path, "%s/%s", pspemu_iso_path, rom.c_str());
        sprintf(sfo_path, "ux0:data/SMLA00001/data/%s/param.sfo", game->id);
        sprintf(icon_path, "ux0:data/SMLA00001/data/%s/icon0.png", game->id);

        if (ISO::isISO(game->rom_path))
        {
            ISO *iso = new ISO(game->rom_path);
            iso->Extract(sfo_path, icon_path);
            delete iso;
        }
        else if (CSO::isCSO(game->rom_path))
        {
            CSO *cso = new CSO(game->rom_path);
            cso->Extract(sfo_path, icon_path);
            delete cso;
        }

        game->type = TYPE_PSP_ISO;
        game->tex = no_icon;
        if (FS::FileExists(sfo_path))
        {
            const auto sfo = FS::Load(sfo_path);
            std::string title = std::string(SFO::GetString(sfo.data(), sfo.size(), "TITLE"));
            std::replace( title.begin(), title.end(), '\n', ' ');
            sprintf(game->title, "%s", title.c_str());

            char* cat = SFO::GetString(sfo.data(), sfo.size(), "CATEGORY");
            char* disc_id = SFO::GetString(sfo.data(), sfo.size(), "DISC_ID");
            if (strcmp(cat, "ME") ==0)
            {
                sprintf(game->category, "%s", game_categories[PS1_GAMES].category);
            }
            else if (strcmp(cat, "UG") ==0 || (disc_id != NULL && IsMatchPrefixes(disc_id, game_categories[PSP_GAMES].valid_title_ids)))
            {
                sprintf(game->category, "%s", game_categories[PSP_GAMES].category);
            }
            else
            {
                sprintf(game->category, "%s", game_categories[PS_MIMI_GAMES].category);
            }
        }
        else
        {
            sprintf(game->title, "%s", rom.substr(0, dot_index).c_str());
            sprintf(game->category, "%s", game_categories[PSP_GAMES].category);
        }        
    }

    void ScanAdrenalineIsoGames(sqlite3 *db)
    {
        sprintf(scan_message, "Scanning for %s games in the %s folder", "ISO", pspemu_iso_path);
        GameCategory *category = &game_categories[PSP_GAMES];
        std::vector<std::string> files = FS::ListFiles(pspemu_iso_path);

        games_to_scan = files.size();
        games_scanned = 0;

        if (!FS::FolderExists("ux0:data/SMLA00001/data"))
        {
            FS::MkDirs("ux0:data/SMLA00001/data");
        }

        for(std::size_t j = 0; j < files.size(); ++j)
        {
            int index = files[j].find_last_of(".");
            if (index != std::string::npos && IsRomExtension(files[j].substr(index), psp_iso_extensions))
            {
                Game game;
                try
                {
                    PopulateIsoGameInfo(&game, files[j], games_scanned);
                    categoryMap[game.category]->current_folder->games.push_back(game);
                    DB::InsertGame(db, &game);
                    game_scan_inprogress = game;
                    games_scanned++;
                }
                catch(const std::exception& e)
                {
                    games_to_scan--;
                }
                
            }
            else
            {
                games_to_scan--;
            }
            
        }
    }

    void PopulateEbootGameInfo(Game *game, std::string rom, int game_index)
    {
        sprintf(game->rom_path, "%s/%s", pspemu_eboot_path, rom.c_str());
        char param_sfo[192];
        sprintf(game->id, "SMLAE%04d", game_index);
        sprintf(param_sfo, "ux0:data/SMLA00001/data/%s/param.sfo", game->id);
        
        EBOOT::Extract(game->rom_path, game->id);

        const auto sfo = FS::Load(param_sfo);
        std::string title = std::string(SFO::GetString(sfo.data(), sfo.size(), "TITLE"));
        std::replace( title.begin(), title.end(), '\n', ' ');
        char* cat = SFO::GetString(sfo.data(), sfo.size(), "CATEGORY");
        char* disc_id = SFO::GetString(sfo.data(), sfo.size(), "DISC_ID");

        game->type = TYPE_EBOOT;
        sprintf(game->title, "%s", title.c_str());
        game->tex = no_icon;
        if (strcmp(cat, "ME") ==0)
        {
            sprintf(game->category, "%s", game_categories[PS1_GAMES].category);
            game_categories[PS1_GAMES].current_folder->games.push_back(*game);
        }
        else if (strcmp(cat, "UG") ==0 || (disc_id != NULL && IsMatchPrefixes(disc_id, game_categories[PSP_GAMES].valid_title_ids)))
        {
            sprintf(game->category, "%s", game_categories[PSP_GAMES].category);
            game_categories[PSP_GAMES].current_folder->games.push_back(*game);
        }
        else
        {
            sprintf(game->category, "%s", game_categories[PS_MIMI_GAMES].category);
            game_categories[PS_MIMI_GAMES].current_folder->games.push_back(*game);
        }
    }

    void ScanAdrenalineEbootGames(sqlite3 *db)
    {
        sprintf(scan_message, "Scanning for %s games in the %s folder", "EBOOT", pspemu_eboot_path);
        std::vector<std::string> files = FS::ListFiles(pspemu_eboot_path);

        games_to_scan = files.size();
        games_scanned = 0;

        for(std::size_t j = 0; j < files.size(); ++j)
        {
            int index = files[j].find_last_of(".");
            if (index != std::string::npos && IsRomExtension(files[j].substr(index), eboot_extensions))
            {
                Game game;
                try
                {
                    PopulateEbootGameInfo(&game, files[j], games_scanned);
                    DB::InsertGame(db, &game);
                    game_scan_inprogress = game;
                    games_scanned++;
                }
                catch(const std::exception& e)
                {
                    games_to_scan--;
                }
                
            }
            else
            {
                games_to_scan--;
            }
        }
    }

    void ScanRetroCategory(sqlite3 *db, GameCategory *category)
    {
        sprintf(scan_message, "Scanning for %s games in the %s folder", category->title, category->roms_path);
        std::vector<std::string> files = FS::ListFiles(category->roms_path);
        games_to_scan = files.size();
        games_scanned = 0;
        int rom_path_length = strlen(category->roms_path);
        sqlite3 *mame_mappings_db;

        if (category->id == MAME_2000_GAMES || category->id == MAME_2003_GAMES || category->id == NEOGEO_GAMES)
        {
            sqlite3_open(MAME_ROM_NAME_MAPPINGS_FILE, &mame_mappings_db);
        }

        int rom_length = 0;
        for(std::size_t j = 0; j < files.size(); j++)
        {
            int dot_index = files[j].find_last_of(".");
            int slash_index = files[j].find_last_of("/");
            rom_length = rom_path_length + files[j].length() + 1;
            if (rom_length < 192 && dot_index != std::string::npos && IsRomExtension(files[j].substr(dot_index), category->file_filters))
            {
                Game game;
                game.type = TYPE_ROM;
                sprintf(game.id, "%s", category->title);
                sprintf(game.category, "%s", category->category);
                sprintf(game.rom_path, "%s/%s", category->roms_path, files[j].c_str());
                if (slash_index != std::string::npos)
                {
                    strlcpy(game.title, files[j].substr(slash_index+1, dot_index-slash_index-1).c_str(), 128);
                }
                else
                {
                    strlcpy(game.title, files[j].substr(0, dot_index).c_str(), 128);
                }
                if (category->id == MAME_2000_GAMES || category->id == MAME_2003_GAMES || category->id == NEOGEO_GAMES)
                {
                    DB::GetMameRomName(mame_mappings_db, game.title, game.title);
                }
                game.tex = no_icon;
                category->current_folder->games.push_back(game);
                DB::InsertGame(db, &game);
                game_scan_inprogress = game;
            }
            games_scanned++;
        }
        if (category->id == MAME_2000_GAMES || category->id == MAME_2003_GAMES)
        {
            sqlite3_close(mame_mappings_db);
        }
    }

    void ScanRetroGames(sqlite3 *db)
    {
        for (int i=0; i<TOTAL_ROM_CATEGORY; i++)
        {
            int category_id = ROM_CATEGORIES[i];
            GameCategory *category = &game_categories[category_id];
            ScanRetroCategory(db, category);
        }
    }

    void SetMaxPage(GameCategory *category)
    {
        for (int i=0; i<category->folders.size(); i++)
        {
            Folder *folder = &category->folders[i];
            folder->max_page = (folder->games.size() + category->games_per_page - 1) / category->games_per_page;
            if (folder->max_page == 0)
            {
                folder->max_page = 1;
            }
        }
    }

    bool Launch(Game *game, BootSettings *settings, char* retro_core) {
        GameCategory* category = categoryMap[game->category];
        if (game->type == TYPE_BUBBLE)
        {
            char uri[35];
            sprintf(uri, "psgm:play?titleid=%s", game->id);
            sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            sceKernelExitProcess(0);
        }
        else if (game->type == TYPE_ROM || (category->id == PS1_GAMES && strcmp(category->rom_launcher_title_id, RETROARCH_TITLE_ID)==0))
        {
            if (category != nullptr)
            {
                if (strcmp(category->rom_launcher_title_id, "RETROVITA") == 0 || category->id == PS1_GAMES)
                {
                    char uri[512];
                    if (retro_core == nullptr)
                    {
                        retro_core = category->core;
                    }
                    sprintf(uri, "psgm:play?titleid=%s&param=%s&param2=%s", RETROARCH_TITLE_ID, retro_core, game->rom_path);
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
        else if (game->type == TYPE_SCUMMVM)
        {
            char uri[512];
            sprintf(uri, "psgm:play?titleid=%s&path=%s&game_id=%s", category->rom_launcher_title_id, game->rom_path, game->id);
            sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            sceKernelDelayThread(1000);
            sceKernelExitProcess(0);
        }
        else if (game->type == TYPE_PSP_ISO || game->type == TYPE_EBOOT)
        {
            char boot_data[320];
            memset(boot_data, 0, sizeof(boot_data));
            boot_data[0] = 0x41;
            boot_data[1] = 0x42;
            boot_data[2] = 0x42;

            boot_data[4] = settings->driver;
            boot_data[8] = settings->execute;
            boot_data[12] = 1;
            boot_data[20] = settings->ps_button_mode;
            boot_data[24] = settings->suspend_threads;
            boot_data[28] = settings->cpu_speed;
            boot_data[32] = settings->plugins;
            boot_data[36] = settings->nonpdrm;
            boot_data[40] = settings->high_memory;

            if (settings->driver != defaul_boot_settings.driver ||
                settings->execute != defaul_boot_settings.execute ||
                settings->ps_button_mode != defaul_boot_settings.ps_button_mode ||
                settings->suspend_threads != defaul_boot_settings.suspend_threads ||
                settings->plugins != defaul_boot_settings.plugins ||
                settings->nonpdrm != defaul_boot_settings.nonpdrm ||
                settings->high_memory != defaul_boot_settings.high_memory ||
                settings->cpu_speed != defaul_boot_settings.cpu_speed)
            {
                boot_data[12] = 0;
            }
            for (int i=0; i<strlen(game->rom_path); i++)
            {
                boot_data[64+i] = game->rom_path[i];
            }
            void* fd;
            if (FS::FileExists(adernaline_launcher_boot_bin_path))
            {
                fd = FS::OpenRW(adernaline_launcher_boot_bin_path);
            }
            else
            {
                fd = FS::Create(adernaline_launcher_boot_bin_path);
            }
            FS::Write(fd, boot_data, 320);
            FS::Close(fd);

            char uri[32];
            sprintf(uri, "psgm:play?titleid=%s", adernaline_launcher_title_id);
            sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            sceKernelExitProcess(0);

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

    void LoadGamesCache(sqlite3 *db) {
        games_to_scan = DB::GetCachedGamesCount(db);
        games_scanned = 0;
        sprintf(scan_message, "%s", "Loading game info from cache");
        DB::GetCachedGames(db);
    };

    void LoadGameImages(int category, int prev_page, int page, int games_per_page) {
        int high = 0;
        int low = 0;

        if (game_categories[category].current_folder->max_page > NUM_CACHED_PAGES + 5)
        {
            int del_page = 0;

            if ((page > prev_page) or (prev_page == game_categories[category].current_folder->max_page && page == 1))
            {
                del_page = DecrementPage(page, NUM_CACHED_PAGES);
            } else if ((page < prev_page) or (prev_page == 1 && page == game_categories[category].current_folder->max_page))
            {
                del_page = IncrementPage(page, NUM_CACHED_PAGES);
            }

            int high = del_page * games_per_page;
            int low = high - games_per_page;
            if (del_page > 0)
            {
                for (int i=low; (i<high && i < game_categories[category].current_folder->games.size()); i++)
                {
                    Game *game = &game_categories[category].current_folder->games[i];
                    if (game->tex.id != no_icon.id)
                    {
                        Tex tmp = game->tex;
                        game->tex = no_icon;
                        Textures::Free(&tmp);
                    }
                }
            }
        }

        high = page * games_per_page;
        low = high - games_per_page;
        for(std::size_t i = low; (i < high && i < game_categories[category].current_folder->games.size()); i++) {
            Game *game = &game_categories[category].current_folder->games[i];
            if (page == current_category->current_folder->page_num && category == current_category->id)
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

    int IncrementCategory(int id, int num_of_ids)
    {
        int new_id = id + num_of_ids;
        if (new_id >= TOTAL_CATEGORY)
        {
            new_id = new_id % TOTAL_CATEGORY;
        }
        return new_id;
    }

    int DecrementCategory(int id, int num_of_ids)
    {
        int new_id = id - num_of_ids;
        if (new_id >= 0)
        {
            return new_id;
        }
        return new_id + TOTAL_CATEGORY;
    }

    int IncrementPage(int page, int num_of_pages)
    {
        int new_page = page + num_of_pages;
        if (new_page > current_category->current_folder->max_page)
        {
            new_page = new_page % current_category->current_folder->max_page;
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
        return new_page + current_category->current_folder->max_page;
    }

    void LoadGameImage(Game *game) {
        Tex tex;
        tex = no_icon;
        debugNetPrintf(DEBUG,"Loding image for game id=%s, type=%d, title=%s\n", game->id, game->type, game->title);
        char icon_path[384];
        if (game->type == TYPE_BUBBLE && strcmp(game->category, game_categories[PS_MOBILE_GAMES].category) == 0)
        {
            sprintf(icon_path, "ur0:appmeta/%s/pic0.png", game->id);
        }
        else if (game->type == TYPE_BUBBLE)
        {
            sprintf(icon_path, "ur0:appmeta/%s/icon0.png", game->id);
        }
        else if (game->type == TYPE_EBOOT || game->type == TYPE_PSP_ISO)
        {
            sprintf(icon_path, "ux0:data/SMLA00001/data/%s/icon0.png", game->id);
        }
        else if (game->type == TYPE_SCUMMVM)
        {
            sprintf(icon_path, "%s/icon0.png", game->rom_path);
        }
        else if (game->type == TYPE_FOLDER)
        {
            GameCategory *cat = categoryMap[game->category];
            Folder* folder = FindFolder(cat, game->folder_id);
            if (folder != nullptr)
            {
                sprintf(icon_path, "%s", folder->icon_path);
            }
            else
            {
                sprintf(icon_path, "ux0:app/SMLA00001/folder.png");
            }
        }
        else if (game->type == TYPE_CATEGORY)
        {
            GameCategory *cat = categoryMap[game->category];
            sprintf(icon_path, cat->category_icon);
        }
        else
        {
            GameCategory* category = categoryMap[game->category];
            std::string rom_path = std::string(game->rom_path);
            int dot_index = rom_path.find_last_of(".");
            if (new_icon_method)
            {
                sprintf(icon_path, "%s.png", rom_path.substr(0, dot_index).c_str());
            }
            else
            {
                int slash_index = rom_path.find_last_of("/");
                std::string rom_name = rom_path.substr(slash_index+1, dot_index-slash_index-1);
                sprintf(icon_path, "%s/%s.png", category->icon_path, rom_name.c_str());
            }
        }
        
        if (game->tex.id == no_icon.id)
        {
            if (Textures::LoadImageFile(icon_path, &tex))
            {
                game->tex = tex;
            }
            else
            {
                game->icon_missing = true;
                game->tex = no_icon;
            }
        }
    }

    int LoadGameImageThread(SceSize args, LoadImagesParams *params)
    {
        int end = params->page_num+params->games_per_page;
        GameCategory *category = &game_categories[params->category];
        if (end > category->current_folder->games.size())
        {
            end = category->current_folder->games.size();
        }
        for (int i=params->page_num; i<end; i++)
        {
            if (category->current_folder->games[i].visible>0 && !category->current_folder->games[i].icon_missing)
            {
                GAME::LoadGameImage(&category->current_folder->games[i]);
                // For concurrency, game might be invisible after being visible.
                if (category->current_folder->games[i].visible == 0 && category->current_folder->games[i].tex.id != no_icon.id)
                {
                    Tex tmp = category->current_folder->games[i].tex;
                    category->current_folder->games[i].tex = no_icon;
                    Textures::Free(&tmp);
                }
            }
        }
        return sceKernelExitDeleteThread(0);
    }

    void StartLoadGameImageThread(int category, int game_num, int games_per_page)
    {
        SceUID load_image_thid = sceKernelCreateThread("load_image_thread", (SceKernelThreadEntry)GAME::LoadGameImageThread, 0x10000100, 0x4000, 0, 0, NULL);
        if (load_image_thid >= 0)
        {
            LoadImagesParams params;
            params.category = category;
            params.page_num = game_num;
            params.games_per_page = games_per_page;
            sceKernelStartThread(load_image_thid, sizeof(LoadImagesParams), &params);
        }
        
    }

    void Exit() {
    }

	void StartLoadImagesThread(int category, int prev_page_num, int page, int games_per_page)
	{
		LoadImagesParams page_param;
        page_param.category = category;
		page_param.prev_page_num = prev_page_num;
		page_param.page_num = page;
        page_param.games_per_page = games_per_page;
		load_images_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::LoadImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (load_images_thid >= 0)
			sceKernelStartThread(load_images_thid, sizeof(LoadImagesParams), &page_param);
	}

    int LoadImagesThread(SceSize args, LoadImagesParams *params) {
        sceKernelDelayThread(300000);
        GAME::LoadGameImages(params->category, params->prev_page_num, params->page_num, params->games_per_page);
        return sceKernelExitDeleteThread(0);
    }

    void StartScanGamesThread()
    {
        scan_games_thid = sceKernelCreateThread("scan_games_thread", (SceKernelThreadEntry)GAME::ScanGamesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (scan_games_thid >= 0)
			sceKernelStartThread(scan_games_thid, 0, NULL);
    }

    int ScanGamesThread(SceSize args, void *argp)
    {
        gui_mode = GUI_MODE_SCAN;
        sceKernelDelayThread(10000);
        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            game_categories[i].current_folder->games.clear();
        }

        GAME::Scan();

        if (game_categories[FAVORITES].current_folder->games.size() > 0)
        {
            current_category = &game_categories[FAVORITES];
        }
        current_category->current_folder->page_num = 1;
        view_mode = current_category->view_mode;
        grid_rows = current_category->rows;
        gui_mode  = GUI_MODE_LAUNCHER;
        if (view_mode == VIEW_MODE_GRID)
        {
            GAME::StartLoadImagesThread(current_category->id, 1, 1, current_category->games_per_page);
        }
        return sceKernelExitDeleteThread(0);
    }

    void StartScanGamesCategoryThread(GameCategory *category)
    {
        ScanGamesParams params;
        params.type = category->rom_type;
        params.category = category->category;
        scan_games_category_thid = sceKernelCreateThread("scan_games_category_thread", (SceKernelThreadEntry)GAME::ScanGamesCategoryThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (scan_games_category_thid >= 0)
			sceKernelStartThread(scan_games_category_thid, sizeof(ScanGamesParams), &params);
    }

    void RemoveGamesFromCategoryByType(sqlite3 *db, GameCategory *category, int rom_type)
    {
        DB::DeleteGamesByCategoryAndType(db, category->category, rom_type);
        for (std::vector<Game>::iterator it=category->current_folder->games.begin(); it!=category->current_folder->games.end(); )
        {
            if (it->type == rom_type)
            {
                category->current_folder->games.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    int ScanGamesCategoryThread(SceSize args, ScanGamesParams *params)
    {
        gui_mode = GUI_MODE_SCAN;
        sceKernelDelayThread(50000);
        sqlite3 *db;
        sqlite3_open(CACHE_DB_FILE, &db);
        if (params->type == TYPE_ROM || strcmp(params->category, "ps1") == 0 || params->type == TYPE_SCUMMVM)
        {
            GameCategory *category = categoryMap[params->category];
            RemoveGamesFromCategoryByType(db, category, params->type);
        }
        
        if (params->type == TYPE_EBOOT || params->type == TYPE_PSP_ISO)
        {
            DB::DeleteGamesByType(db, params->type);
            for (int i=0; i<TOTAL_CATEGORY; i++)
            {
                for (std::vector<Game>::iterator it=game_categories[i].current_folder->games.begin(); it!=game_categories[i].current_folder->games.end(); )
                {
                    if (it->type == params->type)
                    {
                        game_categories[i].current_folder->games.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

        if (params->type == TYPE_ROM  || strcmp(params->category, "ps1") == 0)
        {
            ScanRetroCategory(db, categoryMap[params->category]);
        }
        
        if (params->type == TYPE_PSP_ISO)
        {
            ScanAdrenalineIsoGames(db);
        }
        
        if (params->type == TYPE_EBOOT)
        {
            ScanAdrenalineEbootGames(db);
        }

        if (params->type == TYPE_SCUMMVM)
        {
            ScanScummVMGames(db);
        }

        sqlite3_close(db);

        if (params->type == TYPE_ROM || strcmp(params->category, "ps1") == 0 || params->type == TYPE_SCUMMVM)
        {
            GameCategory *category = categoryMap[params->category];
            category->current_folder->page_num = 1;
            SetMaxPage(category);
            SortGames(category);
            current_category = category;
        }
        
        if (params->type == TYPE_EBOOT || params->type == TYPE_PSP_ISO)
        {
            game_categories[PSP_GAMES].current_folder->page_num = 1;
            SetMaxPage(&game_categories[PSP_GAMES]);
            SortGames(&game_categories[PSP_GAMES]);
            game_categories[PS1_GAMES].current_folder->page_num = 1;
            SetMaxPage(&game_categories[PS1_GAMES]);
            SortGames(&game_categories[PS1_GAMES]);
            game_categories[PS_MIMI_GAMES].current_folder->page_num = 1;
            SetMaxPage(&game_categories[PS_MIMI_GAMES]);
            SortGames(&game_categories[PS_MIMI_GAMES]);
        }
        

        if (current_category->view_mode == VIEW_MODE_GRID)
        {
            GAME::StartLoadImagesThread(current_category->id, 1, 1, current_category->games_per_page);
        }
        gui_mode = GUI_MODE_LAUNCHER;
        return sceKernelExitDeleteThread(0);
    }

    int GameComparator(const void *v1, const void *v2)
    {
        const Game *p1 = (Game *)v1;
        const Game *p2 = (Game *)v2;
        if (p1->type == TYPE_FOLDER && p2->type != TYPE_FOLDER)
        {
            return -1;
        }
        else if (p1->type != TYPE_FOLDER && p2->type == TYPE_FOLDER)
        {
            return 1;
        }

        return strcmp(p1->title, p2->title);
    }

    void DeleteGamesImages(GameCategory *category)
    {
        for (int i=0; i < category->current_folder->games.size(); i++)
        {
            Game *game = &category->current_folder->games[i];
            game->visible = 0;
            game->thread_started = false;
            if (game->tex.id != no_icon.id)
            {
                Tex tmp = game->tex;
                game->tex = no_icon;
                Textures::Free(&tmp);
            }
        }
    }

    Game* FindGame(GameCategory *category, Game *game)
    {
        for (int j=0; j < category->folders.size(); j++)
        {
            Folder* current_folder = &category->folders[j];
            for (int i=0; i < current_folder->games.size(); i++)
            {
                if ((game->type != TYPE_ROM && strcmp(game->id, current_folder->games[i].id) == 0) ||
                    (game->type == TYPE_ROM && strcmp(game->rom_path, current_folder->games[i].rom_path) == 0))
                {
                    return &current_folder->games[i];
                }
            }
        }
        return nullptr;
    }

    int FindGamePosition(GameCategory *category, Game *game)
    {
        for (int i=0; i < category->current_folder->games.size(); i++)
        {
            if ((game->type != TYPE_ROM && game->type != TYPE_SCUMMVM && strcmp(game->id, category->current_folder->games[i].id) == 0) ||
                ((game->type == TYPE_ROM || game->type == TYPE_SCUMMVM) && strcmp(game->rom_path, category->current_folder->games[i].rom_path) == 0))
            {
                return i;
            }
        }
        return -1;
    }

    int RemoveGameFromCategory(GameCategory *category, Game *game)
    {
        for (int j=0; j < category->folders.size(); j++)
        {
            Folder* current_folder = &category->folders[j];
            for (int i=0; i < current_folder->games.size(); i++)
            {
                if ((game->type == TYPE_FOLDER && current_folder->games[i].folder_id == game->folder_id) ||
                    (game->type != TYPE_ROM && game->type != TYPE_SCUMMVM && strcmp(game->id, current_folder->games[i].id) == 0) ||
                    ((game->type == TYPE_ROM || game->type == TYPE_SCUMMVM) && strcmp(game->rom_path, current_folder->games[i].rom_path) == 0))
                {
                    current_folder->games.erase(current_folder->games.begin()+i);
                    return i;
                }
            }
        }
        return -1;
    }

    int RemoveFolderFromCategory(GameCategory *category, int folder_id)
    {
        for (int i=0; i < category->folders.size(); i++)
        {
            if (category->folders[i].id == folder_id)
            {
                category->folders.erase(category->folders.begin()+i);
                return i;
            }
        }
        return -1;
    }

    int RemoveGameFromFolder(Folder *folder, Game *game)
    {
        for (int i=0; i < folder->games.size(); i++)
        {
            if ((game->type == TYPE_FOLDER && folder->games[i].folder_id == game->folder_id) ||
                (game->type != TYPE_ROM && game->type != TYPE_SCUMMVM && strcmp(game->id, folder->games[i].id) == 0) ||
                ((game->type == TYPE_ROM || game->type == TYPE_SCUMMVM) && strcmp(game->rom_path, folder->games[i].rom_path) == 0))
            {
                folder->games.erase(folder->games.begin()+i);
                return i;
            }
        }
        return -1;
    }

    void SortGames(GameCategory *category)
    {
        for (int j=0; j < category->folders.size(); j++)
        {
            Folder* current_folder = &category->folders[j];
            qsort(&current_folder->games[0], current_folder->games.size(), sizeof(Game), GameComparator);
        }
    }

    void SortGames(Folder *folder)
    {
        qsort(&folder->games[0], folder->games.size(), sizeof(Game), GameComparator);
    }

    void RefreshGames(bool all_categories)
    {
        games_to_scan = 1;
        games_scanned = 0;
        sprintf(game_scan_inprogress.title, "%s", "");
        if (all_categories)
        {
            FS::Rm(CACHE_DB_FILE);
            StartScanGamesThread();
        }
        else
        {
            sprintf(scan_message, "Scanning for %s games in the %s folder", current_category->title, current_category->roms_path);
            StartScanGamesCategoryThread(current_category);
        }
        
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
        for (int i=TOTAL_CATEGORY-1; i>0; i--)
        {
            if (IsMatchPrefixes(title_id, game_categories[i].valid_title_ids))
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

    void ScanScummVMGames(sqlite3 *db)
    {
        OpenIniFile(SCUMMVM_INI_FILE);
        int count = GetSectionCount();
        char* sections[count];
        for (int i=0; i<count; i++)
        {
            sections[i] = malloc(64);
        }
        GetSections(sections);

        sprintf(scan_message, "Scanning for SCUMMVM games in the %s file", SCUMMVM_INI_FILE);
        games_to_scan = count;
        games_scanned = 0;

        for (int i=0; i<count; i++)
        {
            Game game;
            char section[64];
            memset(section, 0, 64);
            int len = strlen(sections[i]);
            strncpy(section, sections[i]+1, len-2);
            sprintf(game.rom_path, ReadString(section, SCUMMVM_GAME_PATH, ""));
            if (game.rom_path[0] != 0)
            {
                game.type = TYPE_SCUMMVM;
                sprintf(game.category, game_categories[SCUMMVM_GAMES].category);
                sprintf(game.id, ReadString(section, SCUMMVM_GAME_ID, ""));
                sprintf(game.title, ReadString(section, SCUMMVM_GAME_TITLE, ""));
                game.tex = no_icon;
                game_categories[SCUMMVM_GAMES].current_folder->games.push_back(game);
                DB::InsertGame(db, &game);
                game_scan_inprogress = game;
            }
            games_scanned++;
        }

        for (int i=0; i<count; i++)
        {
            free(sections[i]);
        }

        CloseIniFile();
    }

    void DownloadThumbnail(sqlite3 *database, Game *game)
    {
        std::string title = std::string(game->title);
        std::replace_if(title.begin(), title.end(),
            [](char c) { return !std::isspace(c) && !std::isalnum(c); }, ' ');
        CONFIG::ReplaceAll(title, "'", "''");
        std::vector<std::string> tokens;
        char *token = std::strtok(title.c_str(), " ");
        while (token != NULL) {
            tokens.push_back(token);
            token = std::strtok(NULL, " ");
        }
        sqlite3 *db = database;
        if (database == NULL)
        {
            char db_name[64];
            sprintf(db_name, "ux0:app/SMLA00001/thumbnails/%s.db", game->category);
            int rc = sqlite3_open(db_name, &db);
        }

        char thumbnail[128];
        bool found = DB::FindMatchingThumbnail(db, tokens, thumbnail);
        if (found)
        {
            char url[384];
            char alternate_url[384];
            char path[384];
            char base_url[132];
            char alternate_base_url[132];
            game->icon_missing = false;
            GameCategory *cat = categoryMap[game->category];
            if (cat->icon_type == 1)
            {
                sprintf(base_url, cat->download_url, ICON_TYPE_BOXARTS);
                sprintf(alternate_base_url, cat->download_url, ICON_TYPE_TITLES);
            }
            else
            {
                sprintf(base_url, cat->download_url, ICON_TYPE_TITLES);
                sprintf(alternate_base_url, cat->download_url, ICON_TYPE_BOXARTS);
            }
            
            sprintf(url, "%s/%s", base_url, thumbnail);
            sprintf(alternate_url, "%s/%s", alternate_base_url, thumbnail);
            std::string url_str = std::string(url);
            CONFIG::ReplaceAll(url_str, " ", "%20");
            std::string alternate_url_str = std::string(alternate_url);
            CONFIG::ReplaceAll(alternate_url_str, " ", "%20");

            if (game->type == TYPE_ROM)
            {
                std::string rom_path = std::string(game->rom_path);
                int dot_index = rom_path.find_last_of(".");
                if (new_icon_method)
                {
                    sprintf(path, "%s.png", rom_path.substr(0, dot_index).c_str());
                }
                else
                {
                    int slash_index = rom_path.find_last_of("/");
                    std::string rom_name = rom_path.substr(slash_index+1, dot_index-slash_index-1);
                    sprintf(path, "%s/%s.png", cat->icon_path, rom_name.c_str());
                }
                
            }
            else if (game->type == TYPE_SCUMMVM)
            {
                sprintf(path, "%s/icon0.png", game->rom_path);
            }

            if (!FS::FileExists(path))
            {
                int res = Net::DownloadFile(url_str.c_str(), path);
                if (res < 0)
                {
                    Net::DownloadFile(alternate_url_str.c_str(), path);
                }
            }
        }

        if (database == NULL)
        {
            sqlite3_close(db);
        }
    }

    void DownloadThumbnails(GameCategory *category)
    {
        gui_mode = GUI_MODE_SCAN;
        games_to_scan = category->current_folder->games.size();
        games_scanned = 0;
        sprintf(game_scan_inprogress.title, "%s", "");
        sprintf(scan_message, "Downloading thumbnails for %s games", current_category->title);
        StartDownloadThumbnailsThread(category);
    }

    int DownloadThumbnailsThread(SceSize args, ScanGamesParams *params)
    {
        GameCategory *cat = categoryMap[params->category];
        char db_path[64];
        sprintf(db_path, "%s/%s.db", THUMBNAIL_BASE_PATH, cat->category);
        sqlite3 *db;
        sqlite3_open(db_path, &db);
        for (int i=0; i<cat->current_folder->games.size(); i++)
        {
            if (cat->current_folder->games[i].type == TYPE_ROM || cat->current_folder->games[i].type == TYPE_SCUMMVM)
            {
                DownloadThumbnail(db, &cat->current_folder->games[i]);
                game_scan_inprogress = cat->current_folder->games[i];
            }
            games_scanned++;
        }
        sqlite3_close(db);
        gui_mode = GUI_MODE_LAUNCHER;
        return sceKernelExitDeleteThread(0);
    }

    void StartDownloadThumbnailsThread(GameCategory *category)
    {
        ScanGamesParams params;
        params.type = category->rom_type;
        params.category = category->category;
        download_images_thid = sceKernelCreateThread("download_thumbnails_thread", (SceKernelThreadEntry)GAME::DownloadThumbnailsThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (download_images_thid >= 0)
			sceKernelStartThread(download_images_thid, sizeof(ScanGamesParams), &params);
    }

    int DeleteGamesImagesThread(SceSize args, DeleteImagesParams *params)
    {
        sceKernelDelayThread(5000);
        Folder *folder = params->folder;
        for (int i=0; i < folder->games.size(); i++)
        {
            Game *game = &folder->games[i];
            game->visible = 0;
            game->thread_started = false;
            if (game->tex.id != no_icon.id)
            {
                Tex tmp = game->tex;
                game->tex = no_icon;
                Textures::Free(&tmp);
            }
        }
        return sceKernelExitDeleteThread(0);
    }

    void StartDeleteGameImagesThread(GameCategory *category)
    {
        DeleteImagesParams params;
        params.folder = category->current_folder;
        delete_images_thid = sceKernelCreateThread("delete_images_thread", (SceKernelThreadEntry)GAME::DeleteGamesImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (delete_images_thid >= 0)
			sceKernelStartThread(delete_images_thid, sizeof(DeleteImagesParams), &params);
    }

    void FindGamesByPartialName(std::vector<GameCategory*> &categories, char* search_text, std::vector<Game> &games)
    {
        for (int i=0; i<categories.size(); i++)
        {
            GameCategory *category = categories[i];
            for (int j=0; j < category->folders.size(); j++)
            {
                Folder* current_folder = &category->folders[j];
                for (int k=0; k<current_folder->games.size(); k++)
                {
                    std::string title = std::string(current_folder->games[k].title);
                    std::string text = std::string(search_text);
                    std::transform(title.begin(), title.end(), title.begin(),
                            [](unsigned char c){ return std::tolower(c); });
                    std::transform(text.begin(), text.end(), text.begin(),
                            [](unsigned char c){ return std::tolower(c); });
                    if (title.find(text) != std::string::npos && current_folder->games[k].type != TYPE_FOLDER)
                    {
                        Game game = current_folder->games[k];
                        game.tex = no_icon;
                        games.push_back(game);
                        if (games.size() >= 300)
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

	static int LoadScePaf() {
		static int argp[] = { 0x180000, -1, -1, 1, -1, -1 };

		int result = -1;

		uint32_t buf[4];
		buf[0] = sizeof(buf);
		buf[1] = (uint32_t)&result;
		buf[2] = -1;
		buf[3] = -1;

		return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, buf);
	}

	static int UnloadScePaf() {
		uint32_t buf = 0;
		return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &buf);
	}

	int DeleteApp(const char *titleid)
	{
		int res;
		sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
		
		sceAppMgrDestroyOtherApp();

		res = LoadScePaf();
		if (res < 0)
			goto exit;

		res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
		if (res < 0)
			goto exit;

		res = scePromoterUtilityInit();
		if (res < 0)
			goto exit;

		res = scePromoterUtilityDeletePkg(titleid);
		if (res < 0)
			goto exit;

		res = scePromoterUtilityExit();
		if (res < 0)
			goto exit;

		res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
		if (res < 0)
			goto exit;

exit:
		sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
		res = UnloadScePaf();

		return res;
	}

    void UninstallGame(Game *game)
    {
		int ret = DeleteApp(game->id);
        DB::DeleteVitaAppFolderById(nullptr, game->id);

		if (ret >= 0)
		{
			GameCategory *cat = categoryMap[game->category];
			GAME::RemoveGameFromCategory(cat, game);
			GAME::RemoveGameFromCategory(&game_categories[FAVORITES], game);
			GAME::SetMaxPage(cat);
			GAME::SetMaxPage(&game_categories[FAVORITES]);
			game_uninstalled = 2;
		}
		else
		{
			game_uninstalled = 3;
		}
	}

	void StartUninstallGameThread(Game *game)
    {
        uninstall_game_thid = sceKernelCreateThread("download_thumbnails_thread", (SceKernelThreadEntry)GAME::UninstallGameThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (uninstall_game_thid >= 0)
			sceKernelStartThread(uninstall_game_thid, sizeof(Game), game);
    }

	int UninstallGameThread(SceSize args, Game *game)
	{
		UninstallGame(game);
		return sceKernelExitDeleteThread(0);
	}

    Folder* FindFolder(GameCategory *category, int folder_id)
    {
        for (int i=0; i < category->folders.size(); i++)
        {
            if (category->folders[i].id == folder_id)
            {
                return &category->folders[i];
            }
        }
        return nullptr;
    }

    void MoveGamesBetweenFolders(GameCategory *category, int src_id, int dest_id)
    {
        Folder *src_folder = FindFolder(category, src_id);
        Folder *dest_folder = FindFolder(category, dest_id);

        for (int i=0; i<src_folder->games.size(); i++)
        {
            src_folder->games[i].folder_id = 0;
        }

        dest_folder->games.insert(dest_folder->games.end(), src_folder->games.begin(), src_folder->games.end());
        src_folder->games.clear();
    }

    void ClearSelection(GameCategory *category)
    {
        for (int i=0; i<category->current_folder->games.size(); i++)
        {
            category->current_folder->games[i].selected = false;
        }
    }

    std::vector<Game> GetSelectedGames(GameCategory *category)
    {
        std::vector<Game> list;
        for (int i=0; i<category->current_folder->games.size(); i++)
        {
            if (category->current_folder->games[i].selected)
            {
                list.push_back(category->current_folder->games[i]);
            }
        }
        return list;
    }
}
