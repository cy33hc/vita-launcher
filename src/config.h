#ifndef LAUNCHER_CONFIG_H
#define LAUNCHER_CONFIG_H

#include <string>
#include <vector>
#include "game.h"

#define CONFIG_INI_FILE "ux0:data/SMLA00001/config.ini"

#define VITADB_JSON_FILE "ux0:data/SMLA00001/vitadb.json"
#define VITADB_URL "https://rinnegatamante.it/vitadb/list_hbs_json.php"
#define PSP_ISO_PATH "ux0:pspemu/ISO"
#define PSP_EBOOT_PATH "ux0:pspemu/PSP/GAME"

#define CONFIG_VIEW_MODE "view_mode"
#define CONFIG_ROMS_PATH "roms_path"
#define CONFIG_RETRO_CORE "retro_core"
#define CONFIG_ROM_LAUNCHER_TITLE_ID "rom_launcher_title_id"
#define CONFIG_TITLE_ID_PREFIXES "title_id_prefixes"
#define CONFIG_ICON_PATH "icon_path"
#define CONFIG_ADERNALINE_LAUNCHER_TITLE_ID "adernaline_launcher_title_id"
#define CONFIG_ROM_EXTENSIONS "rom_file_extensions"
#define CONFIG_HIDE_TITLE_IDS "hidden_title_ids"

#define DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID "ADRLANCHR"

#define VIEW_MODE_GRID 0
#define VIEW_MODE_LIST 1

#define CONFIG_GLOBAL "Global"
#define CONFIG_SHOW_ALL_CATEGORIES "show_all_categories"
#define CONFIG_PARENT_CONTROL "parental_control"

// Filters for ROM prefixes during scan
#define PS1_FILTERS ".iso,.bin"
#define NES_FILTERS ".zip,.nes"
#define SNES_FILTERS ".zip,.smc,.sfc"
#define GB_FILTERS ".zip,.gb"
#define GBA_FILTERS ".zip,.gba"
#define N64_FILTERS ".zip,.n64"
#define GBC_FILTERS ".zip,.gbc"
#define NEOGEO_FILTERS ".zip,.bin"
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

static SceUID download_vitadb_thid = -1;
extern bool show_all_categories;
extern bool parental_control;

namespace CONFIG {
    void LoadConfig();
    void ParseTitlePrefixes(const char* prefix_list, std::vector<std::string> &prefixes, bool toLower);
    void SetupCategory(GameCategory *category, int category_id, const char* category_name, const char* core,
                       const char* title_id, const char* code, const char* default_prefixes, const char* default_file_filters,
                       int rom_type);
    void StartDownloadVitaDbThread();
    int DownloadVitaDB(SceSize args, void *argp);
}
#endif
