#ifndef LAUNCHER_CONFIG_H
#define LAUNCHER_CONFIG_H

#include <string>
#include <vector>
#include <algorithm>
#include "game.h"

#define CONFIG_INI_FILE "ux0:data/SMLA00001/config.ini"
#define THUMBNAIL_BASE_PATH "ux0:app/SMLA00001/thumbnails"

#define VITADB_JSON_FILE "ux0:data/SMLA00001/vitadb.json"
#define VITADB_URL "https://rinnegatamante.it/vitadb/list_hbs_json.php"

#define SCUMMVM_INI_FILE "ux0:data/scummvm/scummvm.ini"

#define PSP_ISO_PATH "ux0:pspemu/ISO"
#define PSP_EBOOT_PATH "ux0:pspemu/PSP/GAME"
#define CONFIG_PSPEMU_PATH "pspemu_location"
#define DEFAULT_PSPEMU_PATH "ux0:pspemu"

#define CONFIG_VIEW_MODE "view_mode"
#define CONFIG_ROMS_PATH "roms_path"
#define CONFIG_RETRO_CORE "retro_core"
#define CONFIG_ROM_LAUNCHER_TITLE_ID "rom_launcher_title_id"
#define CONFIG_TITLE_ID_PREFIXES "title_id_prefixes"
#define CONFIG_ICON_PATH "icon_path"
#define CONFIG_ADERNALINE_LAUNCHER_TITLE_ID "adernaline_launcher_title_id"
#define CONFIG_ROM_EXTENSIONS "rom_file_extensions"
#define CONFIG_HIDE_TITLE_IDS "hidden_title_ids"
#define CONFIG_ALT_TITLE "alt_title"
#define CONFIG_ALT_CORES "alternate_cores"
#define CONFIG_CATEGORY_ORDER "category_order"
#define CONFIG_BOOT_WITH_ALT_CORE "boot_with_alt_core"

#define DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID "ADRLANCHR"
#define RETROARCH_TITLE_ID "RETROVITA"

#define VIEW_MODE_GRID 0
#define VIEW_MODE_LIST 1
#define VIEW_MODE_SCROLL 2

#define CONFIG_GLOBAL "Global"
#define CONFIG_SHOW_ALL_CATEGORIES "show_all_categories"
#define CONFIG_PARENT_CONTROL "parental_control"
#define CONFIG_STYLE_NAME "style"
#define CONFIG_DEFAULT_STYLE_NAME "Default"

// Filters for ROM prefixes during scan
#define PS1_FILTERS ".img,.bin,.chd,.ccd,.mdf,.pbp"
#define NES_FILTERS ".zip,.nes"
#define SNES_FILTERS ".zip,.smc,.sfc"
#define GB_FILTERS ".zip,.gb"
#define GBA_FILTERS ".zip,.gba"
#define N64_FILTERS ".zip,.n64"
#define GBC_FILTERS ".zip,.gbc"
#define NEOGEO_FILTERS ".zip,.bin"
#define NEOGEO_PC_FILTERS ".zip"
#define SEGA_SATURN_FILTERS ".zip"
#define GAME_GEAR_FILTERS ".zip,.gg"
#define MASTER_SYSTEM_FILTERS ".zip"
#define MEGA_DRIVE_FILTERS ".zip"
#define NEC_FILTERS ".zip"
#define ATARI_2600_FILTERS ".zip,.a26"
#define ATARI_7800_FILTERS ".zip,.a78"
#define ATARI_LYNX_FILTERS ".zip,.lnx"
#define BANDAI_FILTERS ".zip,.wsc"
#define C64_FILTERS ".zip,.d64"
#define MSX2_FILTERS ".zip,.bin"
#define T_GRAFX_FILTERS ".zip"
#define VECTREX_FILTERS ".zip"
#define GAW_FILTERS ".zip"
#define MAME_2000_FILTERS ".zip"
#define MAME_2003_FILTERS ".zip"
#define SCUMMVM_FILTERS ""

// Filters for title prefixes
#define VITA_TITLE_ID_PREFIXES "PCSA,PCSB,PCSC,PCSD,PCSE,PCSF,PCSG,PCSH,PCSI"
#define PSP_TITLE_ID_PREFIXES "NPEG,NPEH,NPHG,NPHH,NPJG,NPJH,NPJJ,NPUF,NPUG,NPUH,NPUI,NPUJ,UCAS,UCES,UCUS,ULES,ULUS,PSPEM"
#define PS1_TITLE_ID_PREFIXES "NPEE,NPEF,NPHI,NPHJ,NPJI,NPJJ,NPUF,NPUI,NPUJ,PSOEM"
#define PSP_MINI_TITLE_ID_PREFIXES "NPEX,NPEZ,NPHZ,NPUX,NPUZ,PSMEM"
#define PS_MOBILE_TITLE_ID_PREFIXES "NPNA,NPOA,NPPA"
#define NES_TITLE_ID_PREFIXES "NES"
#define SNES_TITLE_ID_PREFIXES "SNES"
#define GB_TITLE_ID_PREFIXES "GB"
#define GBA_TITLE_ID_PREFIXES "GBA"
#define N64_TITLE_ID_PREFIXES "N64"
#define GBC_TITLE_ID_PREFIXES "GBC"
#define NEOGEO_TITLE_ID_PREFIXES "NEOGEO"
#define NEOGEO_PC_TITLE_ID_PREFIXES "NGEOPC"
#define SEGA_SATURN_TITLE_ID_PREFIXES "SSTARN"
#define GAME_GEAR_TITLE_ID_PREFIXES "GGEAR"
#define MASTER_SYSTEM_TITLE_ID_PREFIXES "MSYS"
#define MEGA_DRIVE_TITLE_ID_PREFIXES "MDRIV"
#define NEC_TITLE_ID_PREFIXES "NEC"
#define ATARI_2600_TITLE_ID_PREFIXES "AT2600"
#define ATARI_7800_TITLE_ID_PREFIXES "AT7800"
#define ATARI_LYNX_TITLE_ID_PREFIXES "ATLYNX"
#define BANDAI_TITLE_ID_PREFIXES "BANDAI"
#define C64_TITLE_ID_PREFIXES "C64"
#define MSX2_TITLE_ID_PREFIXES "MSX2"
#define T_GRAFX_TITLE_ID_PREFIXES "TGRAFX"
#define VECTREX_TITLE_ID_PREFIXES "VCTRX"
#define GAW_TITLE_ID_PREFIXES "GAW"
#define MAME_2000_TITLE_ID_PREFIXES "M2000"
#define MAME_2003_TITLE_ID_PREFIXES "M2003"
#define SCUMMVM_TITLE_ID_PREFIXES "SCMVM"

// Alt retroarch cores
#define PS1_ALT_CORES ""
#define NES_ALT_CORES "app0:fceumm_libretro.self,app0:quicknes_libretro.self"
#define SNES_ALT_CORES "app0:snes9x2005_plus_libretro.self,app0:snes9x2010_libretro.self,app0:snes9x2002_libretro.self"
#define GB_ALT_CORES "app0:gambatte_libretro.self,app0:tgbdual_libretro.self"
#define GBA_ALT_CORES "app0:gpsp_libretro.self"
#define N64_ALT_CORES ""
#define GBC_ALT_CORES "app0:tgbdual_libretro.self"
#define NEOGEO_ALT_CORES "app0:fbalpha2012_neo_libretro.self,app0:fbneo_libretro.self"
#define NEOGEO_PC_ALT_CORES "app0:race_libretro.self"
#define SEGA_SATURN_ALT_CORES ""
#define GAME_GEAR_ALT_CORES "app0:genesis_plus_gx_libretro.self"
#define MASTER_SYSTEM_ALT_CORES ""
#define MEGA_DRIVE_ALT_CORES "app0:genesis_plus_gx_libretro.self"
#define NEC_ALT_CORES "app0:mednafen_supergrafx_libretro.self"
#define ATARI_2600_ALT_CORES "app0:stella_libretro.self"
#define ATARI_7800_ALT_CORES ""
#define ATARI_LYNX_ALT_CORES ""
#define BANDAI_ALT_CORES ""
#define C64_ALT_CORES "app0:frodo_libretro.self"
#define MSX2_ALT_CORES ""
#define T_GRAFX_ALT_CORES "app0:mednafen_supergrafx_libretro.self"
#define VECTREX_ALT_CORES ""
#define GAW_ALT_CORES ""
#define MAME_2000_ALT_CORES ""
#define MAME_2003_ALT_CORES "app0:mame2003_libretro.self"

#define SCUMMVM_GAME_ID "gameid"
#define SCUMMVM_GAME_TITLE "description"
#define SCUMMVM_GAME_PATH "path"

#define PS1_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Sony_-_PlayStation/raw/master/Named_Titles"
#define NES_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_Entertainment_System/raw/master/Named_Titles"
#define SNES_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Super_Nintendo_Entertainment_System/raw/master/Named_Titles"
#define GB_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy/raw/master/Named_Titles"
#define GBA_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy_Advance/raw/master/Named_Titles"
#define N64_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Nintendo_64/raw/master/Named_Titles"
#define GBC_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Nintendo_-_Game_Boy_Color/raw/master/Named_Titles"
#define NEOGEO_DOWNLOAD_URL "https://github.com/libretro-thumbnails/SNK_-_Neo_Geo/raw/master/Named_Titles"
#define NEOGEO_PC_DOWNLOAD_URL "https://github.com/libretro-thumbnails/SNK_-_Neo_Geo_Pocket_Color/raw/master/Named_Titles"
#define SEGA_SATURN_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Sega_-_Saturn/raw/master/Named_Titles"
#define GAME_GEAR_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Sega_-_Game_Gear/raw/master/Named_Titles"
#define MASTER_SYSTEM_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Sega_-_Master_System_-_Mark_III/raw/master/Named_Titles"
#define MEGA_DRIVE_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis/raw/master/Named_Titles"
#define NEC_DOWNLOAD_URL "https://github.com/libretro-thumbnails/NEC_-_PC_Engine_CD_-_TurboGrafx-CD/raw/master/Named_Titles"
#define ATARI_2600_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Atari_-_2600/raw/master/Named_Titles"
#define ATARI_7800_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Atari_-_7800/raw/master/Named_Titles"
#define ATARI_LYNX_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Atari_-_Lynx/raw/master/Named_Titles"
#define BANDAI_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Bandai_-_WonderSwan/raw/master/Named_Titles"
#define C64_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Commodore_-_64/raw/master/Named_Titles"
#define MSX2_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Microsoft_-_MSX/raw/master/Named_Titles"
#define T_GRAFX_DOWNLOAD_URL "https://github.com/libretro-thumbnails/NEC_-_PC_Engine_-_TurboGrafx_16/raw/master/Named_Titles"
#define VECTREX_DOWNLOAD_URL "https://github.com/libretro-thumbnails/GCE_-_Vectrex/raw/master/Named_Titles"
#define GAW_DOWNLOAD_URL "https://github.com/libretro-thumbnails/Handheld_Electronic_Game/raw/master/Named_Snaps"
#define MAME_2000_DOWNLOAD_URL "https://github.com/libretro-thumbnails/MAME/raw/master/Named_Titles"
#define MAME_2003_DOWNLOAD_URL "https://github.com/libretro-thumbnails/MAME/raw/master/Named_Titles"
#define SCUMMVM_DOWNLOAD_URL "https://github.com/cy33hc/scummvm_icons/raw/main/icons"

extern bool show_all_categories;
extern bool parental_control;

namespace CONFIG {
    void LoadConfig();
    void RemoveFromMultiValues(std::vector<std::string> &multi_values, std::string value);
    void ParseMultiValueString(const char* prefix_list, std::vector<std::string> &prefixes, bool toLower);
    std::string GetMultiValueString(std::vector<std::string> &multi_values);
    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* core,
                       const char* title_id, const char* code, const char* default_prefixes, const char* default_file_filters,
                       const char* alt_cores, int rom_type, const char* download_url);
    GameCategory GetCategoryConfig(GameCategory *category);
    void SaveCategoryConfig(GameCategory *cat);

    static inline std::string& ltrim(std::string& str, std::string chars)
    {
        str.erase(0, str.find_first_not_of(chars));
        return str;
    }

    static inline std::string& rtrim(std::string& str, std::string chars)
    {
        str.erase(str.find_last_not_of(chars) + 1);
        return str;
    }

    // trim from both ends (in place)
    static inline std::string& trim(std::string& str, std::string chars)
    {
        return ltrim(rtrim(str, chars), chars);
    }

    static inline void ReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
    {
        size_t pos = data.find(toSearch);
        while( pos != std::string::npos)
        {
            data.replace(pos, toSearch.size(), replaceStr);
            pos = data.find(toSearch, pos + replaceStr.size());
        }
    }
}
#endif
