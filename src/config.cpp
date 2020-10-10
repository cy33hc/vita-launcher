#include <vitasdk.h>
#include <string>
#include <cstring>
#include <map>

#include "config.h"
#include "game.h"
#include "json.h"
#include "fs.h"
#include "net.h"

extern "C" {
	#include "inifile.h"
}

bool show_all_categories;
bool parental_control;

using json = nlohmann::json;

namespace CONFIG {

    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* title,
                       const char* title_id, const char* core, const char* default_prefixes)
    {
        const char* valid_title_prefixes;

		category->id = category_id;
        sprintf(category->category, "%s", category_name);
		sprintf(category->title, "%s", title);
		category->page_num = 1;
		category->max_page = 1;
        category->list_view_position = -1;
		category->view_mode = ReadInt(category->title, CONFIG_VIEW_MODE, -1);
        category->opened = false;

        if (core != nullptr)
        {
            sprintf(category->core, "%s", ReadString(category->title, CONFIG_RETRO_CORE, core));
            WriteString(category->title, CONFIG_RETRO_CORE, category->core);
        }

        if (title_id != nullptr)
        {
            sprintf(category->rom_launcher_title_id, "%s", ReadString(category->title, CONFIG_ROM_LAUNCHER_TITLE_ID, title_id));
            WriteString(category->title, CONFIG_ROM_LAUNCHER_TITLE_ID, category->rom_launcher_title_id);
        }

        for (int i=0; i<TOTAL_ROM_CATEGORY; i++)
        {
            if (category->id == ROM_CATEGORIES[i])
            {
                sprintf(category->roms_path, "ux0:roms/%s", category->title);
                sprintf(category->roms_path, "%s", ReadString(category->title, CONFIG_ROMS_PATH, category->roms_path));
                WriteString(category->title, CONFIG_ROMS_PATH, category->roms_path);

                sprintf(category->icon_path, "ux0:roms/%s", category->title);
                sprintf(category->icon_path, "%s", ReadString(category->title, CONFIG_ICON_PATH, category->icon_path));
                WriteString(category->title, CONFIG_ICON_PATH, category->icon_path);
            }
        }

        if (default_prefixes != nullptr)
        {
            valid_title_prefixes = ReadString(category->title, CONFIG_TITLE_ID_PREFIXES, default_prefixes);
            ParseTitlePrefixes(valid_title_prefixes, category->valid_title_ids);
            WriteString(category->title, CONFIG_TITLE_ID_PREFIXES, valid_title_prefixes);
        }
		if (category->view_mode == -1)
		{
			category->view_mode = VIEW_MODE_GRID;
			WriteInt(category->title, CONFIG_VIEW_MODE, VIEW_MODE_GRID);
		}

        categoryMap.insert(std::make_pair(category->category, category));
    }

    void LoadConfig()
    {
		OpenIniFile (CONFIG_INI_FILE);

        // Load global config
        show_all_categories = ReadBool(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, true);
        WriteBool(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, show_all_categories);

        // Load parental control config
        parental_control = ReadBool(CONFIG_GLOBAL, CONFIG_PARENT_CONTROL, false);
        WriteBool(CONFIG_GLOBAL, CONFIG_PARENT_CONTROL, parental_control);

        SetupCategory(&game_categories[VITA_GAMES], VITA_GAMES, "vita", "Vita", nullptr, nullptr, VITA_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PSP_GAMES], PSP_GAMES, "psp", "PSP", nullptr, nullptr, PSP_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS1_GAMES], PS1_GAMES, "ps1", "PSX", "RETROVITA", "app0:pcsx_rearmed_libretro.self", PS1_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS_MIMI_GAMES], PS_MIMI_GAMES, "psmini", "PSP Mini", nullptr, nullptr, PSP_MINI_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PS_MOBILE_GAMES], PS_MOBILE_GAMES, "psmobile", "PS Mobile", nullptr, nullptr, PS_MOBILE_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[NES_GAMES], NES_GAMES, "nes", "NES", "RETROVITA", "app0:nestopia_libretro.self", NES_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[SNES_GAMES], SNES_GAMES, "snes", "SNES", "RETROVITA", "app0:snes9x2005_libretro.self", SNES_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GB_GAMES], GB_GAMES, "gb", "GB", "RETROVITA", "app0:gearboy_libretro.self", GB_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GBA_GAMES], GBA_GAMES, "gba", "GBA", "RETROVITA", "app0:vba_next_libretro.self", GBA_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[N64_GAMES], N64_GAMES, "n64", "N64", "DEDALOX64", nullptr, N64_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GBC_GAMES], GBC_GAMES, "gbc", "GBC", "RETROVITA", "app0:gambatte_libretro.self", GBC_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[NEOGEO_GAMES], NEOGEO_GAMES, "neogeo", "NeoGeo", "RETROVITA", "app0:cap32_libretro.self", NEOGEO_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[GAME_GEAR_GAMES], GAME_GEAR_GAMES, "ggear", "G-Gear", "RETROVITA", "app0:smsplus_libretro.self", GAME_GEAR_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[MASTER_SYSTEM_GAMES], MASTER_SYSTEM_GAMES, "msystem", "M-System", "RETROVITA", "app0:genesis_plus_gx_libretro.self", MASTER_SYSTEM_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[MEGA_DRIVE_GAMES], MEGA_DRIVE_GAMES, "mdrive", "M-Drive", "RETROVITA", "app0:picodrive_libretro.self", MEGA_DRIVE_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[NEC_GAMES], NEC_GAMES, "nec", "NEC", "RETROVITA", "app0:mednafen_pce_fast_libretro.self", NEC_TITLE_ID_PREFIXES);
        SetupCategory(&game_categories[PORT_GAMES], PORT_GAMES, "ports", "Ports", nullptr, nullptr, nullptr);
        SetupCategory(&game_categories[ORIGINAL_GAMES], ORIGINAL_GAMES, "original", "Originals", nullptr, nullptr, nullptr);
        SetupCategory(&game_categories[UTILITIES], UTILITIES, "utilities", "Utilities", nullptr, nullptr, nullptr);
        SetupCategory(&game_categories[EMULATORS], EMULATORS, "emulator", "Emulators", nullptr, nullptr, nullptr);
        SetupCategory(&game_categories[HOMEBREWS], HOMEBREWS, "homebrew", "Others", nullptr, nullptr, nullptr);
        SetupCategory(&game_categories[FAVORITES], FAVORITES, "favorites", "Favorites", nullptr, nullptr, nullptr);

		WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();

        if (!FS::FileExists(VITADB_JSON_FILE))
        {
            sprintf(scan_message, "%s", "Downloading game categories from VitaDB website");
            NET::Download(VITADB_URL, VITADB_JSON_FILE);
        }

        if (FS::FileExists(VITADB_JSON_FILE))
        {
            sprintf(scan_message, "%s", "Extracting games categories from VitaDB json");
            json vitadb = json::parse(FS::Load(VITADB_JSON_FILE));
            int arrayLength = static_cast<int>(vitadb.size());

            for (int i=0; i < arrayLength; i++)
            {
                std::string type = vitadb[i]["type"].get<std::string>();
                if (type == "1")
                {
                    game_categories[PORT_GAMES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
                else if (type == "2")
                {
                    game_categories[ORIGINAL_GAMES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
                else if (type == "4")
                {
                    game_categories[UTILITIES].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                } else if (type == "5")
                {
                    game_categories[EMULATORS].valid_title_ids.push_back(vitadb[i]["titleid"].get<std::string>());
                }
            }
        }

        // Download vitadb only once per day
        if (FS::FileExists(VITADB_JSON_FILE))
        {
            SceIoStat stat;
            SceDateTime date;
            time_t time;
            sceIoGetstat(VITADB_JSON_FILE, &stat);
            sceRtcGetCurrentClockLocalTime(&date);
            if (stat.st_mtime.day != date.day)
            {
                StartDownloadVitaDbThread();
            }
        }
    }

    void ParseTitlePrefixes(const char* prefix_list, std::vector<std::string> &prefixes)
    {
        std::string prefix = "";
        int length = strlen(prefix_list);
        for (int i=0; i<length; i++)
        {
            char c = prefix_list[i];
            if (c != ' ' && c != '\t' && c != ',')
            {
                prefix += c;
            }
            
            if (c == ',' || i==length-1)
            {
                prefixes.push_back(prefix);
                prefix = "";
            }
        }
    }

    void StartDownloadVitaDbThread()
    {
        download_vitadb_thid = sceKernelCreateThread("download_vitadb_thread", (SceKernelThreadEntry)CONFIG::DownloadVitaDB, 0x10000100, 0x4000, 0, 0, NULL);
		if (download_vitadb_thid >= 0)
			sceKernelStartThread(download_vitadb_thid, 0, NULL);
    }

    int DownloadVitaDB(SceSize args, void *argp)
    {
        NET::Download(VITADB_URL, VITADB_JSON_FILE);
        return sceKernelExitDeleteThread(0);
    }
}
