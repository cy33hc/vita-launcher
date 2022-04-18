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
#define VITA_LAUNCHER_VPK_URL "https://github.com/cy33hc/vita-launcher/releases/download/lastest/test_launcher.vpk"
#define VITA_LAUNCHER_VERSION_URL "https://github.com/cy33hc/vita-launcher/releases/download/lastest/version.txt"
#define VITA_LAUNCHER_VERSION_PATH "ux0:app/SMLA00001/version.txt"
#define VITA_LAUNCHER_VERSION_UPDATE_PATH "ux0:data/SMLA00001/tmp/version.txt"
#define VITA_LAUNCHER_VPK_UPDATE_PATH "ux0:data/SMLA00001/tmp/launcher.vpk"

#define YOYO_LOADER_VPK_URL "https://github.com/Rinnegatamante/yoyoloader_vita/releases/download/Nightly/YoYoLoader.vpk"
#define YOYO_LOADER_VPK_UPDATE_PATH "ux0:data/SMLA00001/tmp/yoyoloader.vpk"
#define YOYO_LOADER_VERSION_URL "https://api.github.com/repos/Rinnegatamante/yoyoloader_vita/releases/tags/Nightly"
#define YOYO_LOADER_VERSION_PATH "ux0:app/SMLA00002/yoyo_version.txt"
#define YOYO_LOADER_VERSION_UPDATE_PATH "ux0:data/SMLA00001/tmp/yoyo_version.txt"

#define YOYO_LAUNCHER_VERSION_URL "https://github.com/cy33hc/vita-launcher/releases/download/lastest/yoyo_launcher_version.txt"
#define YOYO_LAUNCHER_VPK_URL "https://github.com/cy33hc/vita-launcher/releases/download/lastest/yoyo_launcher.vpk"
#define YOYO_LAUNCHER_VPK_UPDATE_PATH "ux0:data/SMLA00001/tmp/yoyo_launcher.vpk"
#define YOYO_LAUNCHER_VERSION_PATH "ux0:app/SMLA00002/version.txt"
#define YOYO_LAUNCHER_VERSION_UPDATE_PATH "ux0:data/SMLA00001/tmp/yoyo_launcher_version.txt"

#define ADR_LAUNCHER_VPK_URL "https://github.com/cy33hc/vita-launcher/releases/download/lastest/AdrenalineLauncher.vpk"
#define ADR_LAUNCHER_VPK_UPDATE_PATH "ux0:data/SMLA00001/tmp/AdrenalineLauncher.vpk"

#define SCUMMVM_INI_FILE "ux0:data/scummvm/scummvm.ini"

#define GMS_GAMES_PATH "ux0:data/gms"
#define YOYO_LAUNCH_FILE_PATH "ux0:data/gms/launch.txt"

#define PSP_ISO_PATH "ux0:pspemu/ISO"
#define PSP_EBOOT_PATH "ux0:pspemu/PSP/GAME"
#define CONFIG_PSPEMU_PATH "pspemu_location"
#define DEFAULT_PSPEMU_PATH "ux0:pspemu"
#define PSP_ISO_CACHE_PATH "ux0:pspemu/ISO"
#define PSP_EBOOT_CACHE_PATH "ux0:pspemu/PSP/GAME/_cache"

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
#define CONFIG_SEARCH_TEXT "search_text"
#define CONFIG_GRID_ROWS "grid_rows"
#define CONFIG_NEW_ICON_METHOD "new_icon_method"
#define CONFIG_ICON_TYPE "icon_type"
#define CONFIG_ASPECT_RATIO "icon_aspect_ratio"

#define DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID "ADRLANCHR"
#define RETROARCH_TITLE_ID "RETROVITA"
#define YOYO_LAUNCHER_ID "SMLA00002"
#define ABM_APP_ID "ADRBUBMAN"

#define VIEW_MODE_GRID 0
#define VIEW_MODE_LIST 1
#define VIEW_MODE_SCROLL 2

#define ASPECT_RATIO_4x4 1
#define ASPECT_RATIO_4x3 2
#define ASPECT_RATIO_3x4 3

#define CONFIG_GLOBAL "Global"
#define CONFIG_SHOW_ALL_CATEGORIES "show_all_categories"
#define CONFIG_PARENT_CONTROL "parental_control"
#define CONFIG_STYLE_NAME "style"
#define CONFIG_DEFAULT_STYLE_NAME "Default"
#define CONFIG_SWAP_XO "swap_xo"
#define CONFIG_SHOW_CATEGORY_AS_TABS "show_categories_as_tabs"
#define CONFIG_CATEGORY_ICON "category_icon"
#define CONFIG_STARTUP_CATEGORY "startup_category"
#define CONFIG_DEFAULT_STARTUP_CATEGORY "default"
#define CONFIG_BACKGROUD_MUSIC "backgroud_music"
#define CONFIG_ENABLE_BACKGROUND_MUSIC "enable_backgroud_music"
#define CONFIG_FTP_SERVER_IP "ftp_server_ip"
#define CONFIG_FTP_SERVER_PORT "ftp_server_port"
#define CONFIG_FTP_SERVER_USER "ftp_server_user"
#define CONFIG_FTP_SERVER_PASSWORD "ftp_server_password"
#define CONFIG_FTP_CACHE_PATH "ftp_cache_path"
#define CONFIG_FTP_TRANSFER_MODE "ftp_transfer_mode"
#define CONFIG_BILINEAR_FILTER "enable_bilinear_filter"
#define CONFIG_YOYO_LOADER_SIZE "yoyo_loader_size"

#define ICON_TYPE_BOXARTS "Boxarts"
#define ICON_TYPE_TITLES "Titles"

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
#define ATARI_5200_FILTERS ".zip,.a52"
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
#define NEOGEO_CD_FILTERS ".cue,.chd,.iso"
#define SEGA_32X_FILTERS ".32x,.7z,.bin,.md,.smd,.zip"
#define SEGA_CD_FILTERS ".bin,.chd,.cue,.iso"
#define AMIGA_FILTERS ".hdf"
#define MSX1_FILTERS ".zip,.bin"
#define DREAMCAST_FILTERS ".chd,.cue,.iso"
#define GMS_FILTERS ".apk"

// Filters for title prefixes
#define VITA_TITLE_ID_PREFIXES "PCSA,PCSB,PCSC,PCSD,PCSE,PCSF,PCSG,PCSH,PCSI"
#define SYSTEM_APP_ID_PREFIXES "NPXS"
#define PSP_TITLE_ID_PREFIXES "NPEG,NPEH,NPHG,NPHH,NPJG,NPJH,NPJJ,NPUF,NPUG,NPUH,NPUI,NPUJ,UCAS,UCES,UCUS,ULES,ULUS,PSPEM"
#define PS1_TITLE_ID_PREFIXES "NPEE,NPEF,NPHI,NPHJ,NPJI,NPJJ,NPUF,NPUI,NPUJ,PSOEM,SCUS,SLUS,SLES,SCES,SLPS,SCPS,SLPM"
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
#define NEOGEO_CD_TITLE_ID_PREFIXES "NEOGCD"
#define SEGA_32X_TITLE_ID_PREFIXES "SEG32"
#define SEGA_CD_TITLE_ID_PREFIXES "SEGCD"
#define ATARI_5200_TITLE_ID_PREFIXES "AT5200"
#define AMIGA_TITLE_ID_PREFIXES "AMIGA"
#define MSX1_TITLE_ID_PREFIXES "MSX1"
#define DREAMCAST_TITLE_ID_PREFIXES "DCAST"
#define GMS_TITLE_ID_PREFIXES "GMS"


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
#define NEOGEO_CD_ALT_CORES ""
#define SEGA_32X_ALT_CORES ""
#define SEGA_CD_ALT_CORES "app0:picodrive_libretro.self"
#define ATARI_5200_ALT_CORES ""
#define AMIGA_ALT_CORES ""
#define MSX1_ALT_CORES ""
#define DREAMCAST_ALT_CORES ""

#define SCUMMVM_GAME_ID "gameid"
#define SCUMMVM_GAME_TITLE "description"
#define SCUMMVM_GAME_PATH "path"

#define PS1_DOWNLOAD_URL "https://github.com/cy33hc/Sony_-_PlayStation/raw/master/Named_%s"
#define NES_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Nintendo_Entertainment_System/raw/master/Named_%s"
#define SNES_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Super_Nintendo_Entertainment_System/raw/master/Named_%s"
#define GB_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Game_Boy/raw/master/Named_%s"
#define GBA_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Game_Boy_Advance/raw/master/Named_%s"
#define N64_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Nintendo_64/raw/master/Named_%s"
#define GBC_DOWNLOAD_URL "https://github.com/cy33hc/Nintendo_-_Game_Boy_Color/raw/master/Named_%s"
#define NEOGEO_DOWNLOAD_URL "https://github.com/cy33hc/SNK_-_Neo_Geo/raw/master/Named_%s"
#define NEOGEO_PC_DOWNLOAD_URL "https://github.com/cy33hc/SNK_-_Neo_Geo_Pocket_Color/raw/master/Named_%s"
#define SEGA_SATURN_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Saturn/raw/master/Named_%s"
#define GAME_GEAR_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Game_Gear/raw/master/Named_%s"
#define MASTER_SYSTEM_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Master_System_-_Mark_III/raw/master/Named_%s"
#define MEGA_DRIVE_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Mega_Drive_-_Genesis/raw/master/Named_%s"
#define NEC_DOWNLOAD_URL "https://github.com/cy33hc/NEC_-_PC_Engine_CD_-_TurboGrafx-CD/raw/master/Named_%s"
#define ATARI_2600_DOWNLOAD_URL "https://github.com/cy33hc/Atari_-_2600/raw/master/Named_%s"
#define ATARI_7800_DOWNLOAD_URL "https://github.com/cy33hc/Atari_-_7800/raw/master/Named_%s"
#define ATARI_LYNX_DOWNLOAD_URL "https://github.com/cy33hc/Atari_-_Lynx/raw/master/Named_%s"
#define BANDAI_DOWNLOAD_URL "https://github.com/cy33hc/Bandai_-_WonderSwan/raw/master/Named_%s"
#define C64_DOWNLOAD_URL "https://github.com/cy33hc/Commodore_-_64/raw/master/Named_%s"
#define MSX2_DOWNLOAD_URL "https://github.com/cy33hc/Microsoft_-_MSX2/raw/master/Named_%s"
#define T_GRAFX_DOWNLOAD_URL "https://github.com/cy33hc/NEC_-_PC_Engine_-_TurboGrafx_16/raw/master/Named_%s"
#define VECTREX_DOWNLOAD_URL "https://github.com/cy33hc/GCE_-_Vectrex/raw/master/Named_%s"
#define GAW_DOWNLOAD_URL "https://github.com/cy33hc/Handheld_Electronic_Game/raw/master/Named_%s"
#define MAME_2000_DOWNLOAD_URL "https://github.com/cy33hc/MAME/raw/master/Named_%s"
#define MAME_2003_DOWNLOAD_URL "https://github.com/cy33hc/MAME/raw/master/Named_%s"
#define SCUMMVM_DOWNLOAD_URL "https://github.com/cy33hc/ScummVM-1/raw/master/Named_%s"
#define NEOGEO_CD_DOWNLOAD_URL "https://github.com/cy33hc/SNK_-_Neo_Geo_CD/raw/master/Named_%s"
#define SEGA_32X_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_32X/raw/master/Named_%s"
#define SEGA_CD_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Mega-CD_-_Sega_CD/raw/master/Named_%s"
#define ATARI_5200_DOWNLOAD_URL "https://github.com/cy33hc/Atari_-_5200/raw/master/Named_%s"
#define AMIGA_DOWNLOAD_URL "https://github.com/cy33hc/Commodore_-_Amiga/raw/master/Named_%s"
#define MSX1_DOWNLOAD_URL "https://github.com/cy33hc/Microsoft_-_MSX/raw/master/Named_%s"
#define DREAMCAST_DOWNLOAD_URL "https://github.com/cy33hc/Sega_-_Dreamcast/raw/master/Named_%s"
#define GMS_DOWNLOAD_URL "https://github.com/cy33hc/Game_Maker_Studio/raw/master/Named_%s"

extern bool show_all_categories;
extern bool parental_control;
extern char search_text[];
extern bool new_icon_method;
extern bool swap_xo;
extern bool show_categories_as_tabs;
extern char startup_category[];
extern std::vector<std::string> bg_music_list;
extern bool enable_backgrou_music;
extern char ftp_server_ip[];
extern char ftp_server_user[];
extern char ftp_server_password[];
extern int ftp_server_port;
extern char ftp_cache_path[];
extern bool pasv_mode;
extern bool enable_bilinear_filter;

namespace CONFIG {
    void LoadConfig();
    void RemoveFromMultiValues(std::vector<std::string> &multi_values, std::string value);
    void ParseMultiValueString(const char* prefix_list, std::vector<std::string> &prefixes, bool toLower);
    std::string GetMultiValueString(std::vector<std::string> &multi_values);
    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* core,
                       const char* title_id, const char* code, const char* default_prefixes, const char* default_file_filters,
                       const char* alt_cores, int rom_type, const char* download_url, int grid_rows);
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
