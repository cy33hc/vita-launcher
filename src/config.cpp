#include <vitasdk.h>
#include <string>
#include <cstring>
#include <map>

#include "config.h"
#include "game.h"
#include "fs.h"
#include "style.h"
//#include "debugnet.h"
extern "C" {
	#include "inifile.h"
}

bool show_all_categories;
bool parental_control;
char search_text[32];
bool new_icon_method;

namespace CONFIG {

    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* title,
                       const char* title_id, const char* core, const char* default_prefixes, const char* default_file_filters,
                       const char* default_alt_cores, int rom_type, const char* download_url, int grid_rows)
    {
        const char* valid_title_prefixes;
        const char* file_filters;
        const char* alt_cores;

		category->id = category_id;
        sprintf(category->category, "%s", category_name);
		sprintf(category->title, "%s", title);
		category->page_num = 1;
		category->max_page = 1;
        category->list_view_position = -1;
		category->view_mode = ReadInt(category->title, CONFIG_VIEW_MODE, -1);
        category->opened = false;
        category->rom_type = rom_type;

        category->order = ReadInt(category->title, CONFIG_CATEGORY_ORDER, category_id);
        WriteInt(category->title, CONFIG_CATEGORY_ORDER, category->order);

        category->rows = ReadInt(category->title, CONFIG_GRID_ROWS, 3);
        if (category->rows <= 2)
        {
            category->rows = 2;
            category->columns = 4;
            category->button_size = ImVec2(230,233);
            category->thumbnail_size = ImVec2(220,205);
            category->games_per_page = 8;
        }
        else if (category->rows >= 3)
        {
            category->rows = 3;
            category->columns = 6;
            category->button_size = ImVec2(148,154);
            category->thumbnail_size = ImVec2(138,127);
        }
        category->games_per_page = category->rows * category->columns;

        sprintf(category->alt_title, "%s", ReadString(category->title, CONFIG_ALT_TITLE, category->title));
        WriteString(category->title, CONFIG_ALT_TITLE, category->alt_title);

        category->icon_type = ReadInt(category->title, CONFIG_ICON_TYPE, 1);
        WriteInt(category->title, CONFIG_ICON_TYPE, category->icon_type);

        if (download_url != nullptr)
        {
            sprintf(category->download_url, "%s", download_url);
        }

        if (core != nullptr)
        {
            sprintf(category->core, "%s", ReadString(category->title, CONFIG_RETRO_CORE, core));
            WriteString(category->title, CONFIG_RETRO_CORE, category->core);

            category->boot_with_alt_core = ReadBool(category->title, CONFIG_BOOT_WITH_ALT_CORE, true);
            WriteBool(category->title, CONFIG_BOOT_WITH_ALT_CORE, category->boot_with_alt_core);
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

        if (default_file_filters != nullptr)
        {
            file_filters = ReadString(category->title, CONFIG_ROM_EXTENSIONS, default_file_filters);
            ParseMultiValueString(file_filters, category->file_filters, true);
            WriteString(category->title, CONFIG_ROM_EXTENSIONS, file_filters);
        }

        if (default_prefixes != nullptr)
        {
            valid_title_prefixes = ReadString(category->title, CONFIG_TITLE_ID_PREFIXES, default_prefixes);
            ParseMultiValueString(valid_title_prefixes, category->valid_title_ids, false);
            WriteString(category->title, CONFIG_TITLE_ID_PREFIXES, valid_title_prefixes);
        }

        if (default_alt_cores != nullptr)
        {
            alt_cores = ReadString(category->title, CONFIG_ALT_CORES, default_alt_cores);
            ParseMultiValueString(alt_cores, category->alt_cores, false);
            WriteString(category->title, CONFIG_ALT_CORES, alt_cores);
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
        const char* hidden_title_ids_str;

		OpenIniFile (CONFIG_INI_FILE);

        // Adernaline Default Boot Settings
        defaul_boot_settings.driver = INFERNO;
        defaul_boot_settings.execute = EBOOT_BIN;
        defaul_boot_settings.customized = true;
        defaul_boot_settings.ps_button_mode = MENU;
        defaul_boot_settings.suspend_threads = SUSPEND_YES;
        defaul_boot_settings.plugins = PLUGINS_DISABLE;
        defaul_boot_settings.nonpdrm = NONPDRM_DISABLE;
        defaul_boot_settings.high_memory = HIGH_MEM_DISABLE;
        defaul_boot_settings.cpu_speed = CPU_DEFAULT;

        // setup psp iso extensions
        psp_iso_extensions.push_back(".iso");
        psp_iso_extensions.push_back(".cso");
        eboot_extensions.push_back(".pbp");

        // Load styles
        if (!FS::FolderExists(STYLES_FOLDER))
        {
            FS::MkDirs(STYLES_FOLDER);
        }

        sprintf(search_text, "%s", ReadString(CONFIG_GLOBAL, CONFIG_SEARCH_TEXT, ""));
        WriteString(CONFIG_GLOBAL, CONFIG_SEARCH_TEXT, search_text);
        
        sprintf(pspemu_path, "%s", ReadString(CONFIG_GLOBAL, CONFIG_PSPEMU_PATH, DEFAULT_PSPEMU_PATH));
        WriteString(CONFIG_GLOBAL, CONFIG_PSPEMU_PATH, pspemu_path);
        sprintf(pspemu_iso_path, "%s/ISO", pspemu_path);
        sprintf(pspemu_eboot_path, "%s/PSP/GAME", pspemu_path);

        char* style_value = ReadString(CONFIG_GLOBAL, CONFIG_STYLE_NAME, CONFIG_DEFAULT_STYLE_NAME);
        sprintf(style_name, "%s", style_value);
        WriteString(CONFIG_GLOBAL, CONFIG_STYLE_NAME, style_name);
        Style::SetStylePath(style_name);

        // Load global config
        show_all_categories = ReadBool(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, true);
        WriteBool(CONFIG_GLOBAL, CONFIG_SHOW_ALL_CATEGORIES, show_all_categories);

        new_icon_method = ReadBool(CONFIG_GLOBAL, CONFIG_NEW_ICON_METHOD, false);
        WriteBool(CONFIG_GLOBAL, CONFIG_NEW_ICON_METHOD, new_icon_method);

        // Load parental control config
        parental_control = ReadBool(CONFIG_GLOBAL, CONFIG_PARENT_CONTROL, false);
        WriteBool(CONFIG_GLOBAL, CONFIG_PARENT_CONTROL, parental_control);

        // Config for hidden title ids
        hidden_title_ids_str = ReadString(CONFIG_GLOBAL, CONFIG_HIDE_TITLE_IDS, "");
        ParseMultiValueString(hidden_title_ids_str, hidden_title_ids, false);
        WriteString(CONFIG_GLOBAL, CONFIG_HIDE_TITLE_IDS, hidden_title_ids_str);

        // Load adernaline config
        sprintf(adernaline_launcher_title_id, "%s", ReadString(CONFIG_GLOBAL, CONFIG_ADERNALINE_LAUNCHER_TITLE_ID, DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID));
        WriteString(CONFIG_GLOBAL, CONFIG_ADERNALINE_LAUNCHER_TITLE_ID, adernaline_launcher_title_id);
        sprintf(adernaline_launcher_boot_bin_path, "ux0:app/%s/data/boot.bin", adernaline_launcher_title_id);

        SetupCategory(&game_categories[VITA_GAMES], VITA_GAMES, "vita", "Vita", nullptr, nullptr, VITA_TITLE_ID_PREFIXES, nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[PSP_GAMES], PSP_GAMES, "psp", "PSP", nullptr, nullptr, PSP_TITLE_ID_PREFIXES, nullptr, nullptr, TYPE_PSP_ISO, nullptr, 3);
        SetupCategory(&game_categories[PS1_GAMES], PS1_GAMES, "ps1", "PSX", "RETROVITA", "app0:pcsx_rearmed_libretro.self", PS1_TITLE_ID_PREFIXES, PS1_FILTERS, PS1_ALT_CORES, TYPE_EBOOT, PS1_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[PS_MIMI_GAMES], PS_MIMI_GAMES, "psmini", "PSP Mini", nullptr, nullptr, PSP_MINI_TITLE_ID_PREFIXES, nullptr, nullptr, TYPE_EBOOT, nullptr, 3);
        SetupCategory(&game_categories[PS_MOBILE_GAMES], PS_MOBILE_GAMES, "psmobile", "PS Mobile", nullptr, nullptr, PS_MOBILE_TITLE_ID_PREFIXES, nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[NES_GAMES], NES_GAMES, "nes", "NES", "RETROVITA", "app0:nestopia_libretro.self", NES_TITLE_ID_PREFIXES, NES_FILTERS, NES_ALT_CORES, TYPE_ROM, NES_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[SNES_GAMES], SNES_GAMES, "snes", "SNES", "RETROVITA", "app0:snes9x2005_libretro.self", SNES_TITLE_ID_PREFIXES, SNES_FILTERS, SNES_ALT_CORES, TYPE_ROM, SNES_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[GB_GAMES], GB_GAMES, "gb", "GB", "RETROVITA", "app0:gearboy_libretro.self", GB_TITLE_ID_PREFIXES, GB_FILTERS, GB_ALT_CORES, TYPE_ROM, GB_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[GBA_GAMES], GBA_GAMES, "gba", "GBA", "RETROVITA", "app0:vba_next_libretro.self", GBA_TITLE_ID_PREFIXES, GBA_FILTERS, GBA_ALT_CORES, TYPE_ROM, GBA_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[N64_GAMES], N64_GAMES, "n64", "N64", "DEDALOX64", nullptr, N64_TITLE_ID_PREFIXES, N64_FILTERS, N64_ALT_CORES, TYPE_ROM, N64_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[GBC_GAMES], GBC_GAMES, "gbc", "GBC", "RETROVITA", "app0:gambatte_libretro.self", GBC_TITLE_ID_PREFIXES, GBC_FILTERS, GBC_ALT_CORES, TYPE_ROM, GBC_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[NEOGEO_GAMES], NEOGEO_GAMES, "neogeo", "NeoGeo", "RETROVITA", "app0:cap32_libretro.self", NEOGEO_TITLE_ID_PREFIXES, NEOGEO_FILTERS, NEOGEO_ALT_CORES, TYPE_ROM, NEOGEO_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[NEOGEO_CD_GAMES], NEOGEO_CD_GAMES, "neogeocd", "NeoGeoCD", "RETROVITA", "app0:neocd_libretro.self", NEOGEO_CD_TITLE_ID_PREFIXES, NEOGEO_CD_FILTERS, NEOGEO_CD_ALT_CORES, TYPE_ROM, NEOGEO_CD_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[NEOGEO_PC_GAMES], NEOGEO_PC_GAMES, "neogeopc", "NeoGeoPC", "RETROVITA", "app0:mednafen_ngp_libretro.self", NEOGEO_PC_TITLE_ID_PREFIXES, NEOGEO_PC_FILTERS, NEOGEO_PC_ALT_CORES, TYPE_ROM, NEOGEO_PC_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[SEGA_SATURN_GAMES], SEGA_SATURN_GAMES, "saturn", "S-Saturn", "RETROVITA", "app0:yabause_libretro.self", SEGA_SATURN_TITLE_ID_PREFIXES, SEGA_SATURN_FILTERS, SEGA_SATURN_ALT_CORES, TYPE_ROM, SEGA_SATURN_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[GAME_GEAR_GAMES], GAME_GEAR_GAMES, "ggear", "G-Gear", "RETROVITA", "app0:smsplus_libretro.self", GAME_GEAR_TITLE_ID_PREFIXES, GAME_GEAR_FILTERS, GAME_GEAR_ALT_CORES, TYPE_ROM, GAME_GEAR_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MASTER_SYSTEM_GAMES], MASTER_SYSTEM_GAMES, "msystem", "M-System", "RETROVITA", "app0:genesis_plus_gx_libretro.self", MASTER_SYSTEM_TITLE_ID_PREFIXES, MASTER_SYSTEM_FILTERS, MASTER_SYSTEM_ALT_CORES, TYPE_ROM, MASTER_SYSTEM_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MEGA_DRIVE_GAMES], MEGA_DRIVE_GAMES, "mdrive", "M-Drive", "RETROVITA", "app0:picodrive_libretro.self", MEGA_DRIVE_TITLE_ID_PREFIXES, MEGA_DRIVE_FILTERS, MEGA_DRIVE_ALT_CORES, TYPE_ROM, MEGA_DRIVE_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[SEGA_32X_GAMES], SEGA_32X_GAMES, "sega32x", "Sega32X", "RETROVITA", "app0:picodrive_libretro.self ", SEGA_32X_TITLE_ID_PREFIXES, SEGA_32X_FILTERS, SEGA_32X_ALT_CORES, TYPE_ROM, SEGA_32X_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[SEGA_CD_GAMES], SEGA_CD_GAMES, "segacd", "SegaCD", "RETROVITA", "app0:genesis_plus_gx_libretro.self", SEGA_CD_TITLE_ID_PREFIXES, SEGA_CD_FILTERS, SEGA_CD_ALT_CORES, TYPE_ROM, SEGA_CD_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[NEC_GAMES], NEC_GAMES, "nec", "NEC", "RETROVITA", "app0:mednafen_pce_fast_libretro.self", NEC_TITLE_ID_PREFIXES, NEC_FILTERS, NEC_ALT_CORES, TYPE_ROM, NEC_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[ATARI_2600_GAMES], ATARI_2600_GAMES, "a2600", "A-2600", "RETROVITA", "app0:stella2014_libretro.self", ATARI_2600_TITLE_ID_PREFIXES, ATARI_2600_FILTERS, ATARI_2600_ALT_CORES, TYPE_ROM, ATARI_2600_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[ATARI_5200_GAMES], ATARI_5200_GAMES, "a5200", "A-5200", "RETROVITA", "app0:atari800_libretro.self", ATARI_5200_TITLE_ID_PREFIXES, ATARI_5200_FILTERS, ATARI_5200_ALT_CORES, TYPE_ROM, ATARI_5200_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[ATARI_7800_GAMES], ATARI_7800_GAMES, "a7800", "A-7800", "RETROVITA", "app0:prosystem_libretro.self", ATARI_7800_TITLE_ID_PREFIXES, ATARI_7800_FILTERS, ATARI_7800_ALT_CORES, TYPE_ROM, ATARI_7800_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[ATARI_LYNX_GAMES], ATARI_LYNX_GAMES, "aLynx", "A-Lynx", "RETROVITA", "app0:handy_libretro.self", ATARI_LYNX_TITLE_ID_PREFIXES, ATARI_LYNX_FILTERS, ATARI_LYNX_ALT_CORES, TYPE_ROM, ATARI_LYNX_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[AMIGA_GAMES], AMIGA_GAMES, "amiga", "Amiga", "RETROVITA", "app0:puae_libretro.self", AMIGA_TITLE_ID_PREFIXES, AMIGA_FILTERS, AMIGA_ALT_CORES, TYPE_ROM, AMIGA_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[BANDAI_GAMES], BANDAI_GAMES, "bandai", "Bandai", "RETROVITA", "app0:mednafen_wswan_libretro.self", BANDAI_TITLE_ID_PREFIXES, BANDAI_FILTERS, BANDAI_ALT_CORES, TYPE_ROM, BANDAI_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[C64_GAMES], C64_GAMES, "c64", "C64", "RETROVITA", "app0:vice_x64_libretro.self", C64_TITLE_ID_PREFIXES, C64_FILTERS, C64_ALT_CORES, TYPE_ROM, C64_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MSX1_GAMES], MSX1_GAMES, "msx1", "MSX", "RETROVITA", "app0:fmsx_libretro.self", MSX1_TITLE_ID_PREFIXES, MSX1_FILTERS, MSX1_ALT_CORES, TYPE_ROM, MSX1_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MSX2_GAMES], MSX2_GAMES, "msx2", "MSX2", "RETROVITA", "app0:fmsx_libretro.self", MSX2_TITLE_ID_PREFIXES, MSX2_FILTERS, MSX2_ALT_CORES, TYPE_ROM, MSX2_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[T_GRAFX_GAMES], T_GRAFX_GAMES, "tgrafx", "T-Grafx", "RETROVITA", "app0:mednafen_pce_fast_libretro.self", T_GRAFX_TITLE_ID_PREFIXES, T_GRAFX_FILTERS, T_GRAFX_ALT_CORES, TYPE_ROM, T_GRAFX_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[VECTREX_GAMES], VECTREX_GAMES, "vectrex", "Vectrex", "RETROVITA", "app0:vecx_libretro.self", VECTREX_TITLE_ID_PREFIXES, VECTREX_FILTERS, VECTREX_ALT_CORES, TYPE_ROM, VECTREX_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[GAW_GAMES], GAW_GAMES, "gaw", "GAW", "RETROVITA", "app0:gw_libretro.self", GAW_TITLE_ID_PREFIXES, GAW_FILTERS, GAW_ALT_CORES, TYPE_ROM, GAW_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MAME_2000_GAMES], MAME_2000_GAMES, "mame2k", "MAME2000", "RETROVITA", "app0:mame2000_libretro.self", MAME_2000_TITLE_ID_PREFIXES, MAME_2000_FILTERS, MAME_2000_ALT_CORES, TYPE_ROM, MAME_2000_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[MAME_2003_GAMES], MAME_2003_GAMES, "mame2k3", "MAME2003", "RETROVITA", "app0:mame2003_plus_libretro.self", MAME_2003_TITLE_ID_PREFIXES, MAME_2003_FILTERS, MAME_2003_ALT_CORES, TYPE_ROM, MAME_2003_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[SCUMMVM_GAMES], SCUMMVM_GAMES, "scummvm", "SCUMMVM", "VSCU00001", nullptr, nullptr, nullptr, nullptr, TYPE_SCUMMVM, SCUMMVM_DOWNLOAD_URL, 3);
        SetupCategory(&game_categories[PORT_GAMES], PORT_GAMES, "ports", "Ports", nullptr, nullptr, "", nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[ORIGINAL_GAMES], ORIGINAL_GAMES, "original", "Originals", nullptr, nullptr, "", nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[UTILITIES], UTILITIES, "utilities", "Utilities", nullptr, nullptr, "", nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[EMULATORS], EMULATORS, "emulator", "Emulators", nullptr, nullptr, "", nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[HOMEBREWS], HOMEBREWS, "homebrew", "Homebrews", nullptr, nullptr, nullptr, nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);
        SetupCategory(&game_categories[FAVORITES], FAVORITES, "favorites", "Favorites", nullptr, nullptr, nullptr, nullptr, nullptr, TYPE_BUBBLE, nullptr, 3);

        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void ParseMultiValueString(const char* prefix_list, std::vector<std::string> &prefixes, bool toLower)
    {
        std::string prefix = "";
        int length = strlen(prefix_list);
        for (int i=0; i<length; i++)
        {
            char c = prefix_list[i];
            if (c != ' ' && c != '\t' && c != ',')
            {
                if (toLower)
                {
                    prefix += std::tolower(c);
                }
                else
                {
                    prefix += c;
                }
            }
            
            if (c == ',' || i==length-1)
            {
                prefixes.push_back(prefix);
                prefix = "";
            }
        }
    }

    GameCategory GetCategoryConfig(GameCategory *cat)
    {
        GameCategory category;
        OpenIniFile(CONFIG_INI_FILE);

        category.id = cat->id;
        category.rom_type = cat->rom_type;
        category.view_mode = ReadInt(cat->title, CONFIG_VIEW_MODE, VIEW_MODE_GRID);
        sprintf(category.alt_title, "%s", ReadString(cat->title, CONFIG_ALT_TITLE, ""));
        char* valid_title_prefixes = ReadString(cat->title, CONFIG_TITLE_ID_PREFIXES, "");
        ParseMultiValueString(valid_title_prefixes, category.valid_title_ids, false);

        if (cat->rom_type == TYPE_ROM || cat->id == PS1_GAMES)
        {
            sprintf(category.core, "%s", ReadString(cat->title, CONFIG_RETRO_CORE, ""));
            sprintf(category.rom_launcher_title_id, "%s", ReadString(cat->title, CONFIG_ROM_LAUNCHER_TITLE_ID, ""));
            sprintf(category.roms_path, "%s", ReadString(cat->title, CONFIG_ROMS_PATH, ""));
            sprintf(category.icon_path, "%s", ReadString(cat->title, CONFIG_ICON_PATH, ""));
            char* file_filters = ReadString(cat->title, CONFIG_ROM_EXTENSIONS, "");
            ParseMultiValueString(file_filters, category.file_filters, true);
            char* alt_cores = ReadString(cat->title, CONFIG_ALT_CORES, "");
            ParseMultiValueString(alt_cores, category.alt_cores, false);
        }

        CloseIniFile();

        return category;
    }

    std::string GetMultiValueString(std::vector<std::string> &multi_values)
    {
        std::string vts = std::string("");
        if (multi_values.size() > 0)
        {
            for (int i=0; i<multi_values.size()-1; i++)
            {
                vts.append(multi_values[i].c_str()).append(",");
            }
            vts.append(multi_values[multi_values.size()-1].c_str());
        }
        return vts;
    }

    void SaveCategoryConfig(GameCategory *cat)
    {
        OpenIniFile(CONFIG_INI_FILE);

        WriteInt(cat->title, CONFIG_VIEW_MODE, cat->view_mode);
        WriteString(cat->title, CONFIG_ALT_TITLE, cat->alt_title);
        WriteInt(cat->title, CONFIG_GRID_ROWS, cat->rows);
        if (cat->id != FAVORITES && cat->id != HOMEBREWS)
        {
            WriteString(cat->title, CONFIG_TITLE_ID_PREFIXES, GetMultiValueString(cat->valid_title_ids).c_str());
        }

        if (cat->rom_type == TYPE_ROM || cat->id == PS1_GAMES)
        {
            WriteString(cat->title, CONFIG_RETRO_CORE, cat->core);
            WriteString(cat->title, CONFIG_ROM_LAUNCHER_TITLE_ID, cat->rom_launcher_title_id);
            WriteString(cat->title, CONFIG_ROMS_PATH, cat->roms_path);
            WriteString(cat->title, CONFIG_ICON_PATH, cat->icon_path);
            WriteString(cat->title, CONFIG_ROM_EXTENSIONS, GetMultiValueString(cat->file_filters).c_str());
            WriteString(cat->title, CONFIG_ALT_CORES, GetMultiValueString(cat->alt_cores).c_str());
            WriteBool(cat->title, CONFIG_BOOT_WITH_ALT_CORE, cat->boot_with_alt_core);
            WriteInt(cat->title, CONFIG_ICON_TYPE, cat->icon_type);
        }
        WriteIniFile(CONFIG_INI_FILE);
        CloseIniFile();
    }

    void RemoveFromMultiValues(std::vector<std::string> &multi_values, std::string value)
    {
        auto itr = std::find(multi_values.begin(), multi_values.end(), value);
        if (itr != multi_values.end()) multi_values.erase(itr);
    }
}
