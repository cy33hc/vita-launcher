#ifndef LAUNCHER_GAME_H
#define LAUNCHER_GAME_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include "textures.h"
#include "sqlite3.h"

typedef struct {
    char id[16];
    char title[128];
    char category[10];
    char rom_path[192];
    bool favorite = false;
    char type;
    bool icon_missing = false;
    int visible = 0;
    uint64_t visible_time = 0;
    bool thread_started = false;
    Tex tex;
} Game;

typedef struct {
    char id;
    char title[16];
    char alt_title[32];
    std::vector<Game> games;
    std::vector<std::string> valid_title_ids;
    std::vector<std::string> file_filters;
    char category[10];
    char roms_path[96];
    char icon_path[96];
    char core[64];
    char rom_launcher_title_id[12];
    int max_page;
    int page_num;
    int view_mode;
    std::vector<std::string> alt_cores;
    int list_view_position;
    bool opened;
    int rom_type;
    int order;
} GameCategory;

enum drivers {INFERNO=0, MARCH33=1, NP9660=2};
enum execute {EBOOT_BIN=0, BOOT_BIN=1, EBOOT_OLD=2};
enum psbutton_mode {MENU=0, LIVEAREA=1, STANDARD=2};
enum suspend_threads {SUSPEND_YES=0, SUSPEND_NO=1};
enum plugins {PLUGINS_DEFAULT=0, PLUGINS_ENABLE=1, PLUGINS_DISABLE=2};
enum nonpdrm {NONPDRM_DEFAULT=0, NONPDRM_ENABLE=1, NONPDRM_DISABLE=2};
enum high_memory {HIGH_MEM_DEFAULT=0, HIGH_MEM_ENABLE=1, HIGH_MEM_DISABLE=2};
enum cpu_speed {CPU_DEFAULT=0, CPU_20_10=1, CPU_50_25=2, CPU_75_37=3, CPU_100_50=4, CPU_111_55, CPU_122_61, CPU_133_66,
                CPU_166_83, CPU_200_100, CPU_222_111, CPU_288_144, CPU_300_150, CPU_333_166};

typedef struct {
    drivers driver;
    execute execute;
    bool customized;
    psbutton_mode ps_button_mode;
    suspend_threads suspend_threads;
    plugins plugins;
    nonpdrm nonpdrm;
    high_memory high_memory;
    cpu_speed cpu_speed;
} BootSettings;

#define FAVORITES 0
#define VITA_GAMES 1
#define PSP_GAMES 2
#define PS1_GAMES 3
#define PS_MIMI_GAMES 4
#define PS_MOBILE_GAMES 5
#define NES_GAMES 6
#define SNES_GAMES 7
#define GB_GAMES 8
#define GBC_GAMES 9
#define GBA_GAMES 10
#define N64_GAMES 11
#define NEOGEO_GAMES 12
#define NEOGEO_PC_GAMES 13
#define SEGA_SATURN_GAMES 14
#define GAME_GEAR_GAMES 15
#define MASTER_SYSTEM_GAMES 16
#define MEGA_DRIVE_GAMES 17
#define NEC_GAMES 18
#define ATARI_2600_GAMES 19
#define ATARI_7800_GAMES 20
#define ATARI_LYNX_GAMES 21
#define BANDAI_GAMES 22
#define C64_GAMES 23
#define MSX2_GAMES 24
#define T_GRAFX_GAMES 25
#define VECTREX_GAMES 26
#define GAW_GAMES 27
#define MAME_2000_GAMES 28
#define MAME_2003_GAMES 29
#define PORT_GAMES 30
#define ORIGINAL_GAMES 31
#define UTILITIES 32
#define EMULATORS 33
#define HOMEBREWS 34

#define TOTAL_CATEGORY 35
#define TOTAL_ROM_CATEGORY 25

#define TYPE_BUBBLE 0
#define TYPE_ROM 1
#define TYPE_PSP_ISO 2
#define TYPE_EBOOT 3

extern GameCategory game_categories[];
extern std::map<std::string, GameCategory*> categoryMap;
extern GameCategory *current_category;
extern int games_to_scan;
extern int games_scanned;
extern Game game_scan_inprogress;
extern char scan_message[];
extern int ROM_CATEGORIES[];
extern char adernaline_launcher_boot_bin_path[];
extern char adernaline_launcher_title_id[];
extern BootSettings defaul_boot_settings;
extern std::vector<std::string> psp_iso_extensions;
extern std::vector<std::string> eboot_extensions;
extern std::vector<std::string> hidden_title_ids;
extern char pspemu_path[];
extern char pspemu_iso_path[];
extern char pspemu_eboot_path[];

static SceUID load_images_thid = -1;
static SceUID scan_games_thid = -1;
static SceUID scan_games_category_thid = -1;
static SceUID load_image_thid = -1;

typedef struct LoadImagesParams {
  int category;
  int prev_page_num;
  int page_num;
} LoadImagesParams;

typedef struct ScanGamesParams {
  const char* category;
  int type;
} ScanGamesParams;

namespace GAME {
    int GameComparator(const void *v1, const void *v2);
    void Init();
    void Scan();
    void ScanRetroCategory(sqlite3 *db, GameCategory *category);
    void ScanRetroGames(sqlite3 *db);
    void PopulateIsoGameInfo(Game *game, std::string rom, int game_index);
    void ScanAdrenalineIsoGames(sqlite3 *db);
    void PopulateEbootGameInfo(Game *game, std::string rom, int game_index);
    void ScanAdrenalineEbootGames(sqlite3 *db);
    bool GetGameDetails(const char *id, Game *game);
    bool Launch(Game *game, BootSettings *settings);
    void LoadGamesCache();
    void LoadGameImages(int category, int prev_page, int page_num);
    void LoadGameImage(Game *game);
    void StartLoadGameImageThread(int category, int game_num);
    int LoadGameImageThread(SceSize args, LoadImagesParams *params);
    void Exit();
    int LoadImagesThread(SceSize args, LoadImagesParams *argp);
    int IncrementPage(int page, int num_of_pages);
    int DecrementPage(int page, int num_of_pages);
    void StartLoadImagesThread(int category, int prev_page_num, int page);
    int ScanGamesThread(SceSize args, void *argp);
    void StartScanGamesThread();
    void DeleteGamesImages(GameCategory *category);
    void SetMaxPage(GameCategory *category);
    Game* FindGame(GameCategory *category, Game *game);
    int FindGamePosition(GameCategory *category, Game *game);
    int RemoveGameFromCategory(GameCategory *category, Game *game);
    void SortGames(GameCategory *category);
    void RefreshGames(bool all_categories);
    const char* GetGameCategory(const char *id);
    GameCategory* GetRomCategoryByName(const char* category_name);
    bool IsRomCategory(int categoryId);
    bool IsRomExtension(std::string str, std::vector<std::string> &file_filters);
    std::string str_tolower(std::string s);
    void StartScanGamesCategoryThread(GameCategory *category);
    int ScanGamesCategoryThread(SceSize args, ScanGamesParams *params);
    bool IsMatchPrefixes(const char* id, std::vector<std::string> &prefixes);
    int IncrementCategory(int id, int num_of_ids);
    int DecrementCategory(int id, int num_of_ids);
}

#endif
