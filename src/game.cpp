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
#include "ftpclient.h"

#include "debugnet.h"
extern "C" {
	#include "inifile.h"
}

#define NUM_CACHED_PAGES 4

GameCategory game_categories[TOTAL_CATEGORY+1];
GameCategory* sorted_categories[TOTAL_CATEGORY+1];
std::map<std::string, GameCategory*> categoryMap;
std::vector<std::string> psp_iso_extensions;
std::vector<std::string> eboot_extensions;
std::vector<std::string> hidden_title_ids;
char pspemu_path[32];
char gms_data_path[96];
char pspemu_iso_path[36];
char pspemu_eboot_path[41];
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
char adernaline_launcher_config_bin_path[50];
char adernaline_launcher_title_id[12];
BootSettings default_boot_settings;
FtpClient *ftpclient;
int64_t bytes_transfered;
int64_t bytes_to_download;
bool download_error;
char download_error_message[512];
bool use_game_db = true;

namespace GAME {

    void Init() {
        ftpclient = new FtpClient();
        ftpclient->SetConnmode(pasv_mode ? FtpClient::pasv : FtpClient::port);
        ftpclient->SetCallbackBytes(1);
        ftpclient->SetCallbackXferFunction(DownloadGameCallback);
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

        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            SortGames(&game_categories[i]);
            game_categories[i].current_folder->page_num = 1;
            SetMaxPage(&game_categories[i]);
        }
        
        if (show_all_categories)
        {
            ShowAllCategories();
        }
        else
        {
            HideCategories();
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

    int PopulateIsoGameInfo(Game *game, std::string rom, int game_index)
    {
        sprintf(game->id, "%s%04d", "SMLAP", game_index);
        char sfo_path[192];
        char icon_path[192];
        sprintf(sfo_path, "ux0:data/SMLA00001/data/%s", game->id);
        FS::MkDirs(sfo_path);
        sprintf(game->rom_path, "%s", rom.c_str());
        sprintf(sfo_path, "ux0:data/SMLA00001/data/%s/param.sfo", game->id);
        sprintf(icon_path, "ux0:data/SMLA00001/data/%s/icon0.png", game->id);

        if (rom.find_first_of("ftp0:") == 0)
        {
            int dot_index = rom.find_last_of(".");
            std::string remote_sfo_path = rom.substr(5, dot_index-5) + ".sfo";
            std::string remote_icon_path = rom.substr(5, dot_index-5) + ".png";
            int64_t file_size;
            if (ftpclient->Size(remote_sfo_path.c_str(), &file_size, FtpClient::image) > 0)
            {
                ftpclient->Get(sfo_path, remote_sfo_path.c_str(), FtpClient::image, 0);
            }
            if (ftpclient->Size(remote_icon_path.c_str(), &file_size, FtpClient::image) > 0)
            {
                ftpclient->Get(icon_path, remote_icon_path.c_str(), FtpClient::image, 0);
            }
        }
        else
        {
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
            game->cache_state = 2;
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
            return -1;
        }

        return 0;
    }

    void ScanAdrenalineIsoGames(sqlite3 *db)
    {
        std::vector<std::string> psp_iso_paths;
        psp_iso_paths.push_back(std::string(PSP_ISO_PATH));
        psp_iso_paths.push_back(std::string(pspemu_iso_path));
        int game_index = 0;

        if (!FS::FolderExists("ux0:data/SMLA00001/data"))
        {
            FS::MkDirs("ux0:data/SMLA00001/data");
        }

        for (int iso_path_index=0; iso_path_index < psp_iso_paths.size(); iso_path_index++)
        {
            // if paths are the same, then don't continue
            if (iso_path_index > 0 && psp_iso_paths[iso_path_index] == psp_iso_paths[iso_path_index-1])
            {
                break;
            }

            sprintf(scan_message, "Scanning for %s games in the %s folder", "ISO", psp_iso_paths[iso_path_index].c_str());
            std::vector<std::string> files = GetRomFiles(psp_iso_paths[iso_path_index]);

            games_to_scan = files.size();
            games_scanned = 0;

            if (psp_iso_paths[iso_path_index].find_first_of("ftp0:") == 0)
            {
                int ret = ftpclient->Connect(ftp_server_ip, ftp_server_port);
                if (ret > 0)
                {
                    ftpclient->Login(ftp_server_user, ftp_server_password);
                }
            }

            for(std::size_t j = 0; j < files.size(); ++j)
            {
                game_index++;
                int index = files[j].find_last_of(".");
                if (index != std::string::npos && IsRomExtension(files[j].substr(index), psp_iso_extensions) && files[j].find_first_of("cache_") != 0)
                {
                    Game game;
                    try
                    {
                        int ret = PopulateIsoGameInfo(&game, psp_iso_paths[iso_path_index] + "/" + files[j], game_index);
                        if (ret == 0)
                        {
                            categoryMap[game.category]->current_folder->games.push_back(game);
                            DB::InsertGame(db, &game);
                            game_scan_inprogress = game;
                        }
                        games_scanned++;
                    }
                    catch(const std::exception& e)
                    {
                        games_scanned++;
                    }
                    
                }
                else
                {
                    games_scanned++;
                }
                
            }

            if (psp_iso_paths[iso_path_index].find_first_of("ftp0:") == 0)
            {
                ftpclient->Quit();
            }
        }
    }

    int PopulateEbootGameInfo(Game *game, std::string rom, int game_index)
    {
        char param_sfo[192];
        char icon_path[192];

        sprintf(game->id, "SMLAE%04d", game_index);
        sprintf(param_sfo, "ux0:data/SMLA00001/data/%s", game->id);
        if (!FS::FolderExists(param_sfo))
        {
            FS::MkDirs(param_sfo);
        }

        sprintf(param_sfo, "ux0:data/SMLA00001/data/%s/param.sfo", game->id);
        sprintf(icon_path, "ux0:data/SMLA00001/data/%s/icon0.png", game->id);
        sprintf(game->rom_path, "%s", rom.c_str());
        
        if (rom.find_first_of("ftp0:") == 0)
        {
            int slash_index = rom.find_last_of("/");
            std::string remote_sfo_path = rom.substr(5, slash_index-5) + "/param.sfo";
            std::string remote_icon_path = rom.substr(5, slash_index-5) + "/icon0.png";
            int64_t file_size;
            if (ftpclient->Size(remote_sfo_path.c_str(), &file_size, FtpClient::image) > 0)
            {
                if (ftpclient->Get(param_sfo, remote_sfo_path.c_str(), FtpClient::image, 0) == 0)
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
            if (ftpclient->Size(remote_icon_path.c_str(), &file_size, FtpClient::image) > 0)
            {
                ftpclient->Get(icon_path, remote_icon_path.c_str(), FtpClient::image, 0);
            }
        }
        else
        {
            int ret = EBOOT::Extract(game->rom_path, game->id);
            if (ret != 0)
            {
                return -1;
            }
        }

        const auto sfo = FS::Load(param_sfo);
        std::string title = std::string(SFO::GetString(sfo.data(), sfo.size(), "TITLE"));
        std::replace( title.begin(), title.end(), '\n', ' ');
        char* cat = SFO::GetString(sfo.data(), sfo.size(), "CATEGORY");
        char* disc_id = SFO::GetString(sfo.data(), sfo.size(), "DISC_ID");

        game->type = TYPE_EBOOT;
        sprintf(game->title, "%s", title.c_str());
        game->tex = no_icon;
        game->cache_state = 2;
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

        return 0;
    }

    void ScanAdrenalineEbootGames(sqlite3 *db)
    {
        std::vector<std::string> psp_eboot_paths;
        psp_eboot_paths.push_back(std::string(PSP_EBOOT_PATH));
        psp_eboot_paths.push_back(std::string(pspemu_eboot_path));
        int game_index = 0;

        if (!FS::FolderExists("ux0:data/SMLA00001/data"))
        {
            FS::MkDirs("ux0:data/SMLA00001/data");
        }

        for (int eboot_path_index=0; eboot_path_index < psp_eboot_paths.size(); eboot_path_index++)
        {
            // if paths are the same, then don't continue
            if (eboot_path_index > 0 && psp_eboot_paths[eboot_path_index] == psp_eboot_paths[eboot_path_index-1])
            {
                break;
            }

            sprintf(scan_message, "Scanning for %s games in the %s folder", "EBOOT", psp_eboot_paths[eboot_path_index].c_str());
            std::vector<std::string> files = GetRomFiles(psp_eboot_paths[eboot_path_index]);

            games_to_scan = files.size();
            games_scanned = 0;

            if (psp_eboot_paths[eboot_path_index].find_first_of("ftp0:") == 0)
            {
                int ret = ftpclient->Connect(ftp_server_ip, ftp_server_port);
                if (ret > 0)
                {
                    ftpclient->Login(ftp_server_user, ftp_server_password);
                }
            }

            for(std::size_t j = 0; j < files.size(); ++j)
            {
                game_index++;
                int index = files[j].find_last_of(".");
                if (index != std::string::npos && IsRomExtension(files[j].substr(index), eboot_extensions) && files[j].find_first_of("_cache/") != 0)
                {
                    Game game;
                    try
                    {
                        int ret = PopulateEbootGameInfo(&game, psp_eboot_paths[eboot_path_index] + "/" + files[j], game_index);
                        if (ret == 0)
                        {
                            DB::InsertGame(db, &game);
                            game_scan_inprogress = game;
                        }
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

            if (psp_eboot_paths[eboot_path_index].find_first_of("ftp0:") == 0)
            {
                ftpclient->Quit();
            }
        }
    }

    std::vector<std::string> GetRomFiles(const std::string path)
    {
        std::vector<std::string> files;
        if (strncmp(path.c_str(), "ftp0:", 5) == 0)
        {
            ftpclient->Connect(ftp_server_ip, ftp_server_port);
            if (ftpclient->Login(ftp_server_user, ftp_server_password) > 0)
            {
                files =  ftpclient->ListFiles(path.substr(5).c_str(), true);
            }
            ftpclient->Quit();
        }
        else
        {
            files = FS::ListFiles(path);
        }

        return files;
    }

    void ScanRetroCategory(sqlite3 *db, GameCategory *category)
    {
        sprintf(scan_message, "Scanning for %s games in the %s folder", category->title, category->roms_path);
        std::vector<std::string> files = GetRomFiles(category->roms_path);
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
                game.cache_state = 2;
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
            if (strncmp(game->id, "NPXS", 4)==0)
            {
                sceAppMgrLaunchAppByUri(0x40000, uri);
            }
            else
            {
                sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            }
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
                    std::string game_path = std::string(game->rom_path);
                    if (game_path.rfind("ftp0:", 0) == 0)
                    {
                        game_path = std::string(ftp_cache_path) + "/" + game->category + "/" + game_path.substr(5);
                    }
                    sprintf(uri, "psgm:play?titleid=%s&param=%s&param2=%s", RETROARCH_TITLE_ID, retro_core, game_path.c_str());
                    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
                    sceKernelDelayThread(1000);
                    sceKernelExitProcess(0);
                }
                else if (strcmp(category->rom_launcher_title_id, "DEDALOX64") == 0)
                {
                    char uri[512];
                    std::string game_path = std::string(game->rom_path);
                    if (game_path.rfind("ftp0:", 0) == 0)
                    {
                        game_path = std::string(ftp_cache_path) + "/" + game->category + "/" + game_path.substr(5);
                    }
                    sprintf(uri, "psgm:play?titleid=%s&param=%s", category->rom_launcher_title_id, game_path.c_str());
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
        else if (game->type == TYPE_GMS)
        {
            char uri[512];
            std::string game_path = std::string(game->rom_path);
            sprintf(uri, "psgm:play?titleid=%s&param=%s", YOYO_LAUNCHER_ID, game_path.substr(game_path.find_last_of("/")+1).c_str());
            sceAppMgrLaunchAppByUri(0xFFFFF, uri);
            sceKernelDelayThread(1000);
            sceKernelExitProcess(0);
            //sceAppMgrLoadExec(settings->video_support ? "app0:/loader2.bin" : "app0:/loader.bin", NULL, NULL);
            return 0;
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

            if (settings->driver != default_boot_settings.driver ||
                settings->execute != default_boot_settings.execute ||
                settings->ps_button_mode != default_boot_settings.ps_button_mode ||
                settings->suspend_threads != default_boot_settings.suspend_threads ||
                settings->plugins != default_boot_settings.plugins ||
                settings->nonpdrm != default_boot_settings.nonpdrm ||
                settings->high_memory != default_boot_settings.high_memory ||
                settings->cpu_speed != default_boot_settings.cpu_speed)
            {
                boot_data[12] = 0;
            }
            std::string rom_path = std::string(game->rom_path);
            char rom_path_temp[192];
            if (rom_path.find_first_of("ftp0:") == 0)
            {
                if (game->type == TYPE_PSP_ISO)
                {
                    rom_path = std::string(PSP_ISO_CACHE_PATH) + "/cache_" + rom_path.substr(rom_path.find_last_of("/")+1);
                    sprintf(rom_path_temp, "%s", rom_path.c_str());
                }
                else
                {
                    int last_slash = rom_path.find_last_of("/");
                    std::string eboot_name = rom_path.substr(last_slash);
                    std::string eboot_folder_name = rom_path.substr(0, last_slash);
                    eboot_folder_name = eboot_folder_name.substr(eboot_folder_name.find_last_of("/"));
                    sprintf(rom_path_temp, "%s%s%s", PSP_EBOOT_CACHE_PATH, eboot_folder_name.c_str(), eboot_name.c_str());
                }
            }
            else
            {
                sprintf(rom_path_temp, "%s", rom_path.c_str());
            }

            for (int i=0; i<strlen(rom_path_temp); i++)
            {
                boot_data[64+i] = rom_path_temp[i];
            }
            void* fd;
            if (FS::FileExists(adernaline_launcher_config_bin_path))
            {
                FS::Rm(adernaline_launcher_config_bin_path);
            }

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

    int GetSortedCategoryIndex(int category_id)
    {
        for (int i=0; i<TOTAL_CATEGORY; i++)
        {
            if (sorted_categories[i]->id == category_id)
            {
                return i;
            }
        }
    }

    int IncrementCategory(int category_id, int num_of_ids)
    {
        int index = GetSortedCategoryIndex(category_id);

        int new_id = index + num_of_ids;
        if (new_id >= TOTAL_CATEGORY)
        {
            new_id = new_id % TOTAL_CATEGORY;
        }
        return new_id;
    }

    int DecrementCategory(int category_id, int num_of_ids)
    {
        int index = GetSortedCategoryIndex(category_id);

        int new_id = index - num_of_ids;
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

        char icon_path[384];
        if (game->type == TYPE_BUBBLE && strcmp(game->category, game_categories[PS_MOBILE_GAMES].category) == 0)
        {
            sprintf(icon_path, "ur0:appmeta/%s/pic0.png", game->id);
        }
        else if (game->type == TYPE_BUBBLE)
        {
            if (strncmp(game->id, "NPXS", 4)==0)
            {
                sprintf(icon_path, "vs0:app/%s/sce_sys/icon0.png", game->id);
            }
            else
            {
                sprintf(icon_path, "ur0:appmeta/%s/icon0.png", game->id);
            }
        }
        else if (game->type == TYPE_EBOOT || game->type == TYPE_PSP_ISO)
        {
            sprintf(icon_path, "ux0:data/SMLA00001/data/%s/icon0.png", game->id);
        }
        else if (game->type == TYPE_SCUMMVM)
        {
            sprintf(icon_path, "%s/icon0.png", game->rom_path);
        }
        else if (game->type == TYPE_GMS)
        {
            std::string tmp_path = std::string(game->rom_path);
            sprintf(icon_path, "%s/%s/icon0.png", GMS_GAMES_PATH, tmp_path.substr(tmp_path.find_last_of("/")+1).c_str());
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
        Folder *folder = category->current_folder;
        if (end > folder->games.size())
        {
            end = folder->games.size();
        }
        for (int i=params->page_num; i<end; i++)
        {
            if (folder->games[i].visible>0 && !folder->games[i].icon_missing)
            {
                GAME::LoadGameImage(&folder->games[i]);
                // For concurrency, game might be invisible after being visible.
                if (folder->games[i].visible == 0 && folder->games[i].tex.id != no_icon.id)
                {
                    Tex tmp = folder->games[i].tex;
                    folder->games[i].tex = no_icon;
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
        free(ftpclient);
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
        gui_mode = GUI_MODE_SCAN;
        scan_games_thid = sceKernelCreateThread("scan_games_thread", (SceKernelThreadEntry)GAME::ScanGamesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (scan_games_thid >= 0)
			sceKernelStartThread(scan_games_thid, 0, NULL);
    }

    int ScanGamesThread(SceSize args, void *argp)
    {
        sceKernelDelayThread(5000);
        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            game_categories[i].current_folder->games.clear();
        }

        GAME::Scan();

        if (strcmp(startup_category, CONFIG_DEFAULT_STARTUP_CATEGORY)==0)
        {
            if (show_categories_as_tabs)
            {
                if (game_categories[FAVORITES].current_folder->games.size() > 0)
                {
                    current_category = &game_categories[FAVORITES];
                }
            }
            else
            {
                current_category = &game_categories[CATEGORY];
            }
        }
        else
        {
            current_category = categoryMap[startup_category];
        }
        
        current_category->current_folder->page_num = 1;
        view_mode = current_category->view_mode;
        grid_rows = current_category->rows;
        aspect_ratio = current_category->ratio;
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
        for (int i=0; i<category->folders.size(); i++)
        {
            for (std::vector<Game>::iterator it=category->folders[i].games.begin(); it!=category->folders[i].games.end(); )
            {
                if (it->type == rom_type)
                {
                    category->folders[i].games.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        category->current_folder = &category->folders[0];
    }

    int ScanGamesCategoryThread(SceSize args, ScanGamesParams *params)
    {
        gui_mode = GUI_MODE_SCAN;
        sceKernelDelayThread(50000);
        sqlite3 *db;
        sqlite3_open(CACHE_DB_FILE, &db);
        if (params->type == TYPE_ROM || params->type == TYPE_SCUMMVM || params->type == TYPE_GMS)
        {
            GameCategory *category = categoryMap[params->category];
            RemoveGamesFromCategoryByType(db, category, params->type);
        }

        if (strcmp(params->category, "ps1") == 0)
        {
            GameCategory *category = categoryMap[params->category];
            RemoveGamesFromCategoryByType(db, category, TYPE_ROM);
        }
        
        if (params->type == TYPE_EBOOT || params->type == TYPE_PSP_ISO)
        {
            DB::DeleteGamesByType(db, params->type);
            for (int i=0; i<TOTAL_CATEGORY; i++)
            {
                for (int j=0; j<game_categories[i].folders.size(); j++)
                {
                    for (std::vector<Game>::iterator it=game_categories[i].folders[j].games.begin(); it!=game_categories[i].folders[j].games.end(); )
                    {
                        if (it->type == params->type)
                        {
                            game_categories[i].folders[j].games.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }
                }
            }
            game_categories[PSP_GAMES].current_folder = &game_categories[PSP_GAMES].folders[0];
            game_categories[PS1_GAMES].current_folder = &game_categories[PS1_GAMES].folders[0];
            game_categories[PS_MIMI_GAMES].current_folder = &game_categories[PS_MIMI_GAMES].folders[0];
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

        if (params->type == TYPE_GMS)
        {
            ScanGMSGames(db);
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

    int GameCategoryComparator(const void *v1, const void *v2)
    {
        const Game *p1 = (Game *)v1;
        const Game *p2 = (Game *)v2;
        GameCategory *c1 = categoryMap[p1->category];
        GameCategory *c2 = categoryMap[p2->category];

        return CategoryComparator(&c1, &c2);
    }

    int CategoryComparator(const void *v1, const void *v2)
    {
        GameCategory *p1 = *(GameCategory **)v1;
        GameCategory *p2 = *(GameCategory **)v2;
        if (p1->id == FAVORITES || p2->id == CATEGORY)
        {
            return -1;
        }
        if (p2->id == FAVORITES || p1->id == CATEGORY)
        {
            return 1;
        }

        if (p1->order < p2->order)
        {
            return -1;
        }
        else if (p1->order > p2->order)
        {
            return 1;
        }
        else
        {
            return strcmp(p1->alt_title, p2->alt_title);
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

    void SortCategories()
    {
        qsort(&sorted_categories[0], TOTAL_CATEGORY+1, sizeof(GameCategory*), CategoryComparator);
    }

    void SortGameCategories()
    {
        qsort(&game_categories[CATEGORY].folders[0].games[0], game_categories[CATEGORY].folders[0].games.size(), sizeof(Game), GameCategoryComparator);
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

    std::vector<std::string> GetGMSRomFiles(const std::string path)
    {
        std::vector<std::string> files;
        if (strncmp(path.c_str(), "ftp0:", 5) == 0)
        {
            std::vector<FtpDirEntry> ftp_files = ftpclient->ListDir(path.substr(5).c_str());
            for (int i=0; i<ftp_files.size(); i++)
            {
                files.push_back(std::string(ftp_files[i].name));
            }
        }
        else
        {
            files = FS::ListDir(path);
        }

        return files;
    }

    void ScanGMSGames(sqlite3 *db)
    {
        sprintf(scan_message, "Scanning for GMS games in the %s folder", GMS_GAMES_PATH);

        bool is_ftp_enabled = strncmp(game_categories[GMS_GAMES].roms_path, "ftp0:", 5) == 0;
        bool is_ftp_connected;
        std::vector<std::string> files;
        if (is_ftp_enabled)
        {
            ftpclient->Connect(ftp_server_ip, ftp_server_port);
            if (ftpclient->Login(ftp_server_user, ftp_server_password) > 0)
            {
                is_ftp_connected = true;
                files = GetGMSRomFiles(game_categories[GMS_GAMES].roms_path);
            }
            else
            {
                is_ftp_connected = false;
                files = std::vector<std::string>();
            }
        }
        else
        {
            files = GetGMSRomFiles(game_categories[GMS_GAMES].roms_path);
        }
        games_to_scan = files.size();
        games_scanned = 0;

        char rom_path[512];
        char icon_path[512];
        char icon_local_path[192];
        int rom_length;
        bool rom_exists;
        for(std::size_t j = 0; j < files.size(); j++)
        {
            sprintf(rom_path, "%s/%s/game.apk", game_categories[GMS_GAMES].roms_path, files[j].c_str());
            rom_length = strlen(rom_path);

            if (is_ftp_enabled)
            {
                if (is_ftp_connected)
                {
                    std::string tmp_path = std::string(rom_path);
                    int64_t file_size;
                    rom_exists = ftpclient->Size(tmp_path.substr(5).c_str(), &file_size, FtpClient::image);
                }
                else
                {
                    rom_exists = false;
                }
            }
            else
            {
                rom_exists = FS::FileExists(rom_path);
            }

            if (rom_length < 192 && rom_exists)
            {
                Game game;
                game.type = TYPE_GMS;
                game.cache_state = 2;
                sprintf(game.title, "%s", files[j].c_str());
                sprintf(game.id, "%s", game_categories[GMS_GAMES].title);
                sprintf(game.category, "%s", game_categories[GMS_GAMES].category);
                sprintf(game.rom_path, "%s/%s", game_categories[GMS_GAMES].roms_path, files[j].c_str());
                game.tex = no_icon;
                game_categories[GMS_GAMES].current_folder->games.push_back(game);
                DB::InsertGame(db, &game);

                // download icon if ftp enabled
                if (is_ftp_enabled && is_ftp_connected)
                {
                    sprintf(icon_path, "%s/%s/icon0.png", game_categories[GMS_GAMES].roms_path, game.title);
                    sprintf(icon_local_path, "%s/%s/icon0.png", GMS_GAMES_PATH, game.title);
                    FS::MkDirs(std::string(GMS_GAMES_PATH) + "/" + game.title);
                    if (!FS::FileExists(icon_local_path))
                    {
                        ftpclient->Get(icon_local_path, std::string(icon_path).substr(5).c_str(), FtpClient::image, 0);
                    }
                }
                game_scan_inprogress = game;
            }
            games_scanned++;
        }

        if (is_ftp_enabled)
        {
            ftpclient->Quit();
        }
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

        char thumbnail[133];
        bool found = DB::FindMatchingThumbnail(db, tokens, thumbnail);
        if (!found)
        {
            sprintf(thumbnail, "%s.png", game->title);
        }

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
        else if (game->type == TYPE_GMS)
        {
            std::string rom_path = std::string(game->rom_path);
            sprintf(path, "%s/%s/icon0.png", GMS_GAMES_PATH, rom_path.substr(rom_path.find_last_of("/")+1).c_str());
        }

        if (!FS::FileExists(path))
        {
            int res = Net::DownloadFile(url_str.c_str(), path);
            if (res < 0)
            {
                Net::DownloadFile(alternate_url_str.c_str(), path);
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
        FS::MkDirs(cat->icon_path);
        for (int i=0; i<cat->current_folder->games.size(); i++)
        {
            if (cat->current_folder->games[i].type == TYPE_ROM ||
                cat->current_folder->games[i].type == TYPE_SCUMMVM ||
                cat->current_folder->games[i].type == TYPE_GMS )
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
        sceKernelDelayThread(params->timeout);
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

    void StartDeleteGameImagesThread(GameCategory *category, int timeout)
    {
        DeleteImagesParams params;
        params.folder = category->current_folder;
        params.timeout = timeout;
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

	int DeleteApp(const char *titleid)
	{
		int res;
		sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
		
        //res = scePromoterUtilityInit();
        //if (res < 0)
        //    return res;

		sceAppMgrDestroyOtherApp();

		res = scePromoterUtilityDeletePkg(titleid);

		sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        //res = scePromoterUtilityExit();
        //if (res < 0)
        //    return res;

		return res;
	}

    void UninstallGame(Game *game)
    {
		int ret = DeleteApp(game->id);
        DB::DeleteVitaAppFolderById(nullptr, game->id);
        DB::DeleteFavorite(nullptr, game);

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

    void ClearCategories()
    {
        for (std::vector<Game>::iterator it=game_categories[TOTAL_CATEGORY].folders[0].games.begin();
              it!=game_categories[TOTAL_CATEGORY].folders[0].games.end(); )
        {
            game_categories[TOTAL_CATEGORY].folders[0].games.erase(it);
            if (it->tex.id != no_icon.id)
            {
                Textures::Free(&it->tex);
            }
        }
    }

    void HideCategories()
    {
        ClearCategories();

        for (int i=0; i < TOTAL_CATEGORY; i++)
        {
            if (game_categories[i].current_folder->games.size() > 0)
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
            }
        }

        SortGameCategories();
    }

    void ShowAllCategories()
    {
        ClearCategories();

        game_categories[TOTAL_CATEGORY].current_folder->games.clear();

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
        }

        SortGameCategories();
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

    char GetCacheState(Game *game)
    {
        if (game->cache_state != 2)
        {
            return game->cache_state;
        }
        else if (IsRemoteGame(game))
        {
            if (game->type == TYPE_ROM)
            {
                std::string game_path = std::string(game->rom_path);
                game->cache_state = FS::FileExists(std::string(ftp_cache_path) + "/" + game->category + "/" + game_path.substr(5));
                return game->cache_state;
            }
            else if (game->type == TYPE_PSP_ISO)
            {
                std::string game_path = std::string(game->rom_path);
                game->cache_state = FS::FileExists(std::string(PSP_ISO_CACHE_PATH) + "/cache_" + game_path.substr(game_path.find_last_of("/")+1));
                return game->cache_state;
            }
            else if (game->type == TYPE_EBOOT)
            {
                std::string game_path = std::string(game->rom_path);
                int last_slash = game_path.find_last_of("/");
                std::string eboot_name = game_path.substr(last_slash);
                std::string eboot_folder_name = game_path.substr(0, last_slash);
                eboot_folder_name = eboot_folder_name.substr(eboot_folder_name.find_last_of("/"));
                game->cache_state = FS::FileExists(std::string(PSP_EBOOT_CACHE_PATH) + eboot_folder_name + eboot_name);
                return game->cache_state;
            }
            else if (game->type == TYPE_GMS)
            {
                std::string game_path = std::string(game->rom_path);
                char rom_local_path[192];
                sprintf(rom_local_path, "%s/%s/game.apk", GMS_GAMES_PATH, game_path.substr(game_path.find_last_of("/")+1).c_str());
                game->cache_state = FS::FileExists(rom_local_path);
            }
        }
        else
        {
            return game->cache_state;
        }
    }

    void DeleteCachedRoms(GameCategory *category)
    {
        gui_mode = GUI_MODE_SCAN;
        games_to_scan = category->current_folder->games.size();
        games_scanned = 0;
        sprintf(game_scan_inprogress.title, "%s", "");
        sprintf(scan_message, "Delete cached roms for %s games", category->title);
        StartDeleteCachedRomsThread(category);
    }

    void DeleteCachedRom(Game *game)
    {
        if (IsRemoteGame(game))
        {
            if (game->type == TYPE_ROM)
            {
                std::string game_path = std::string(game->rom_path);
                FS::Rm(std::string(ftp_cache_path) + "/" + game->category + "/" + game_path.substr(5));
                game->cache_state = 0;
            }
            else if (game->type == TYPE_PSP_ISO)
            {
                std::string game_path = std::string(game->rom_path);
                FS::Rm(std::string(PSP_ISO_CACHE_PATH) + "/cache_" + game_path.substr(game_path.find_last_of("/")+1));
            }
            else if (game->type == TYPE_EBOOT)
            {
                std::string game_path = std::string(game->rom_path);
                int last_slash = game_path.find_last_of("/");
                std::string eboot_name = game_path.substr(last_slash);
                std::string eboot_folder_name = game_path.substr(0, last_slash);
                eboot_folder_name = eboot_folder_name.substr(eboot_folder_name.find_last_of("/"));
                FS::Rm(std::string(PSP_EBOOT_CACHE_PATH) + eboot_folder_name + eboot_name);
            }
            game->cache_state = 0;
        }
    }

    int DeleteCachedRomsThread(SceSize args, ScanGamesParams *params)
    {
        GameCategory *cat = categoryMap[params->category];
        for (int i=0; i<cat->current_folder->games.size(); i++)
        {
            DeleteCachedRom(&cat->current_folder->games[i]);
            Game *favorite_game = FindGame(&game_categories[FAVORITES], &cat->current_folder->games[i]);
            if (favorite_game != nullptr)
            {
                favorite_game->cache_state = 0;
            }
            game_scan_inprogress = cat->current_folder->games[i];
            games_scanned++;
        }
        gui_mode = GUI_MODE_LAUNCHER;
        return sceKernelExitDeleteThread(0);
    }

    void StartDeleteCachedRomsThread(GameCategory *category)
    {
        ScanGamesParams params;
        params.type = category->rom_type;
        params.category = category->category;
        delete_cached_roms_thid = sceKernelCreateThread("delete_cached_roms_thread", (SceKernelThreadEntry)GAME::DeleteCachedRomsThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (delete_cached_roms_thid >= 0)
			sceKernelStartThread(delete_cached_roms_thid, sizeof(ScanGamesParams), &params);
    }

    bool IsRemoteGame(Game *game)
    {
        if (strncmp(game->rom_path, "ftp0:", 5) == 0)
        {
            return true;
        }
        return false;
    }

    void DownloadGameToFtpCache(Game *game)
    {
        bytes_transfered = 0;
        bytes_to_download = 1000;
        download_error = false;
        std::string path = std::string(game->rom_path);
        if (path.rfind("ftp0:", 0) == 0)
        {
            if (ftpclient->Connect(ftp_server_ip, ftp_server_port) > 0)
            {
                if (ftpclient->Login(ftp_server_user, ftp_server_password) > 0)
                {
                    if (game->type == TYPE_ROM)
                    {
                        std::string cache_path = std::string(ftp_cache_path) + "/" + game->category + "/" + path.substr(5);
                        std::string cache_path_prefix = cache_path.substr(0, cache_path.find_last_of("/")+1);
                        path = path.substr(5);
                        std::string path_prefix = path.substr(0, path.find_last_of("/")+1);
                        FS::MkDirs(cache_path_prefix);
                        if (ftpclient->Size(path.c_str(), &bytes_to_download, FtpClient::image) > 0)
                        {
                            if (ftpclient->Get(cache_path.c_str(), path.c_str(), FtpClient::image, 0) > 0)
                            {
                                game->cache_state = 1;
                                if (cache_path.rfind(".cue") != std::string::npos || cache_path.rfind(".CUE") != std::string::npos)
                                {
                                    std::vector<std::string> files = GetFilesFromCueFile(cache_path.c_str());
                                    for (int i=0; i<files.size(); i++)
                                    {
                                        bytes_to_download = 1000;
                                        bytes_transfered = 0;
                                        std::string output_file = cache_path_prefix + files[i];
                                        std::string input_file = path_prefix + files[i];
                                        if (ftpclient->Size(input_file.c_str(), &bytes_to_download, FtpClient::image) > 0)
                                        {
                                            if (ftpclient->Get(output_file.c_str(), input_file.c_str(), FtpClient::image, 0) == 0)
                                            {
                                                download_error = true;
                                                game->cache_state = 0;
                                                break;
                                            }
                                            else
                                            {
                                                game->cache_state = 1;
                                            }
                                        }
                                        else
                                        {
                                            download_error = true;
                                            game->cache_state = 0;
                                            break;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                download_error = true;
                            }
                        }
                        else
                        {
                            download_error = true;
                        }
                    }
                    else if (game->type == TYPE_PSP_ISO)
                    {
                        if (!FS::FolderExists(PSP_ISO_CACHE_PATH))
                        {
                            FS::MkDirs(PSP_ISO_CACHE_PATH);
                        }
                        std::string output_file = std::string(PSP_ISO_CACHE_PATH) + "/cache_" + path.substr(path.find_last_of("/")+1);
                        path = path.substr(5);
                        if (ftpclient->Size(path.c_str(), &bytes_to_download, FtpClient::image) > 0)
                        {
                            if (ftpclient->Get(output_file.c_str(), path.c_str(), FtpClient::image, 0) > 0)
                            {
                                game->cache_state = 1;
                            }
                            else
                            {
                                download_error = true;
                            }
                        }
                        else
                        {
                            download_error = true;
                        }
                    }
                    else if (game->type == TYPE_EBOOT)
                    {
                        if (!FS::FolderExists(PSP_EBOOT_CACHE_PATH))
                        {
                            FS::MkDirs(PSP_EBOOT_CACHE_PATH);
                        }
                        int last_slash = path.find_last_of("/");
                        std::string eboot_name = path.substr(last_slash);
                        std::string eboot_folder_name = path.substr(0, last_slash);
                        eboot_folder_name = eboot_folder_name.substr(eboot_folder_name.find_last_of("/"));
                        FS::MkDirs(std::string(PSP_EBOOT_CACHE_PATH) + eboot_folder_name);
                        std::string output_file = std::string(PSP_EBOOT_CACHE_PATH) + eboot_folder_name + eboot_name;
                        path = path.substr(5);
                        if (ftpclient->Size(path.c_str(), &bytes_to_download, FtpClient::image) > 0)
                        {
                            if (ftpclient->Get(output_file.c_str(), path.c_str(), FtpClient::image, 0) > 0)
                            {
                                game->cache_state = 1;
                            }
                            else
                            {
                                download_error = true;
                            }
                        }
                        else
                        {
                            download_error = true;
                        }
                    }
                    else if (game->type == TYPE_GMS)
                    {
                        std::string output_path = std::string(GMS_GAMES_PATH) + "/" + path.substr(path.find_last_of("/")+1);
                        if (!FS::FolderExists(output_path.c_str()))
                        {
                            FS::MkDirs(output_path.c_str());
                        }
                        output_path = output_path + "/game.apk";
                        path = path.substr(5) + "/game.apk";
                        if (ftpclient->Size(path.c_str(), &bytes_to_download, FtpClient::image) > 0)
                        {
                            if (ftpclient->Get(output_path.c_str(), path.c_str(), FtpClient::image, 0) > 0)
                            {
                                game->cache_state = 1;
                            }
                            else
                            {
                                download_error = true;
                            }
                        }
                        else
                        {
                            download_error = true;
                        }
                    }
                }
                else
                {
                    download_error = true;
                }
            }
            else
            {
                download_error = true;
            }
            
            if (download_error)
            {
                strncpy(download_error_message, ftpclient->LastResponse(), strlen(ftpclient->LastResponse()));
            }
            ftpclient->Quit();
        }
    }

	void StartDownloadGameThread(Game *game)
    {
        download_game_thid = sceKernelCreateThread("download_game_thread", (SceKernelThreadEntry)GAME::DownloadGameThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (download_game_thid >= 0)
			sceKernelStartThread(download_game_thid, sizeof(Game*), &game);
    }

	int DownloadGameThread(SceSize args, Game **game)
	{
		DownloadGameToFtpCache(*game);
		return sceKernelExitDeleteThread(0);
	}

    std::vector<std::string> GetFilesFromCueFile(char *path)
    {
        std::vector<std::string> out;
        FILE *fp = fopen(path, "r");
        char buf[1024];
        char *p;
        char *token;

        while (fgets(buf, 1024, fp) != NULL)
        {
            token = strtok_r(buf, " \t", &p);
            if (token == NULL || strcmp(token, "FILE") != 0)
            {
                continue;
            }

            // read up to quote
            token = strtok_r(NULL, "\"", &p);
            if (token == NULL)
            {
                continue;
            }
            out.push_back(std::string(token));
        }
        fclose(fp);

        return out;
    }

    void StartGetCacheStateThread()
    {
        get_cachestate_thid = sceKernelCreateThread("get_cachestate_thid", (SceKernelThreadEntry)GAME::GetCacheStateThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (get_cachestate_thid >= 0)
			sceKernelStartThread(get_cachestate_thid, 0, NULL);
    }

    int GetCacheStateThread(SceSize args, void *argp)
    {
        while (gui_mode == GUI_MODE_SCAN)
        {
            sceKernelDelayThread(5000);
        }

        for (int i=PSP_GAMES; i < TOTAL_CATEGORY; i++)
        {
            Folder *folder = game_categories[i].current_folder;
            for (int j=0; j < folder->games.size(); j++)
            {
                GetCacheState(&folder->games[j]);
            }
        }

        return sceKernelExitDeleteThread(0);
    }

    static int DownloadGameCallback(int64_t xfered, void* arg)
    {
        bytes_transfered = xfered;
        return 1;
    }

    void MigratePSPCache()
    {
        std::string old_cache_path = std::string(PSP_ISO_PATH) + "/_cache";
        if (FS::FolderExists(old_cache_path))
        {
            std::vector<std::string> files = GetRomFiles(old_cache_path);
            for (int i=0; i<files.size(); i++)
            {
                std::string old_path = old_cache_path + "/" + files[i];
                std::string new_path = std::string(PSP_ISO_CACHE_PATH) + "/cache_" + files[i];
                if (!FS::FileExists(new_path))
                {
                    FS::Rename(old_path, new_path);
                }
                else
                {
                    FS::Rm(old_path);
                }
            }

            FS::RmDir(old_cache_path);
        }
    }

    void LoadYoYoSettings(Game *game, BootSettings *settings)
    {
        char configFile[512];
        char buffer[30];
        int value;
        
        std::string rom_path = std::string(game->rom_path);
        sprintf(configFile, "%s/%s/yyl.cfg", GMS_GAMES_PATH, rom_path.substr(rom_path.find_last_of("/")+1).c_str());
        FILE *config = fopen(configFile, "r");

        settings->bilinear = false;
        settings->compress_textures = false;
        settings->debug_mode = false;
        settings->debug_shaders = false;
        settings->fake_win_mode = false;
        settings->gles1 = false;
        settings->mem_extended = false;
        settings->newlib_extended = false;
        settings->skip_splash = false;
        settings->video_support = false;
        if (config) {
            while (EOF != fscanf(config, "%[^=]=%d\n", buffer, &value)) {
                if (strcmp("forceGLES1", buffer) == 0) settings->gles1 = (bool)value;
                else if (strcmp("forceBilinear", buffer) == 0) settings->bilinear = (bool)value;
                else if (strcmp("winMode", buffer) == 0) settings->fake_win_mode = (bool)value;
                else if (strcmp("debugShaders", buffer) == 0) settings->debug_shaders = (bool)value;
                else if (strcmp("compressTextures", buffer) == 0) settings->compress_textures = (bool)value;
                else if (strcmp("debugMode", buffer) == 0) settings->debug_mode = (bool)value;
                else if (strcmp("noSplash", buffer) == 0) settings->skip_splash = (bool)value;
                else if (strcmp("maximizeMem", buffer) == 0) settings->mem_extended = (bool)value;
                else if (strcmp("maximizeNewlib", buffer) == 0) settings->newlib_extended = (bool)value;
                else if (strcmp("videoSupport", buffer) == 0) settings->video_support = (bool)value;
            }
            fclose(config);
        }
    }

    void SaveYoYoSettings(Game *game, BootSettings *settings)
    {
        char configFile[512];
        char buffer[128];
        std::string rom_path = std::string(game->rom_path);
        std::string game_name = rom_path.substr(rom_path.find_last_of("/")+1);
        sprintf(configFile, "%s/%s/yyl.cfg", GMS_GAMES_PATH, game_name.c_str());

        void *f = FS::OpenRW(configFile);
        sprintf(buffer, "%s=%d\n", "forceGLES1", settings->gles1);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "noSplash", settings->skip_splash);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "forceBilinear", settings->bilinear);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "winMode", settings->fake_win_mode);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "compressTextures", settings->compress_textures);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "debugMode", settings->debug_mode);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "debugShaders", settings->debug_shaders);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "maximizeMem", settings->mem_extended);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "maximizeNewlib", settings->newlib_extended);
        FS::Write(f, buffer, strlen(buffer));
        sprintf(buffer, "%s=%d\n", "videoSupport", settings->video_support);
        FS::Write(f, buffer, strlen(buffer));
        FS::Close(f);
    }
}
