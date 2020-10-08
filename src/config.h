#ifndef LAUNCHER_CONFIG_H
#define LAUNCHER_CONFIG_H

#include <string>
#include <vector>
#include "game.h"

#define CONFIG_INI_FILE "ux0:data/SMLA00001/config.ini"

#define VITADB_JSON_FILE "ux0:data/SMLA00001/vitadb.json"
#define VITADB_URL "https://rinnegatamante.it/vitadb/list_hbs_json.php"

#define CONFIG_VIEW_MODE "view_mode"
#define CONFIG_ROMS_PATH "roms_path"
#define CONFIG_RETRO_CORE "retro_core"
#define CONFIG_ROM_LAUNCHER_TITLE_ID "rom_launcher_title_id"
#define CONFIG_TITLE_ID_PREFIXES "title_id_prefixes"

#define VIEW_MODE_GRID 0
#define VIEW_MODE_LIST 1

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

namespace CONFIG {
    void LoadConfig();
    void ParseTitlePrefixes(const char* prefix_list, std::vector<std::string> &prefixes);
    void SetupCategory(GameCategory *category, int category_id, const char* category_name,
                       const char* core, const char* title_id, const char* code, const char* default_prefixes);
}
#endif
