#ifndef LAUNCHER_GAME_H
#define LAUNCHER_GAME_H

#pragma once
#include <imgui_vita2d/imgui_vita.h>
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "textures.h"
#include "sqlite3.h"
#include "ftpclient.h"

typedef struct {
    char id[20];
    char title[128];
    char category[10];
    char rom_path[192];
    bool favorite = false;
    char type;
    bool icon_missing = false;
    int visible = 0;
    int folder_id = 0;
    uint64_t visible_time = 0;
    bool thread_started = false;
    bool selected = false;
    char cache_state = 2;
    Tex tex;
} Game;

typedef struct {
    int id = -1;
    char title[128];
    char category[10];
    char icon_path[192];
    char type;
    int max_page;
    int page_num;
    std::vector<Game> games;
} Folder;

typedef struct {
    char id;
    char title[32];
    char alt_title[32];
    std::vector<Folder> folders;
    Folder *current_folder;
    std::vector<std::string> valid_title_ids;
    std::vector<std::string> file_filters;
    char category[10];
    char roms_path[96];
    char icon_path[96];
    char category_icon[96];
    char core[64];
    char rom_launcher_title_id[12];
    int view_mode;
    std::vector<std::string> alt_cores;
    bool boot_with_alt_core;
    int list_view_position;
    bool opened;
    int rom_type;
    int order;
    char download_url[256];
    int rows;
    int columns;
    int ratio;
    int games_per_page;
    ImVec2 button_size;
    ImVec2 thumbnail_size;
    ImVec2 thumbnail_offset;
    ImVec2 normal_thumbnail_size;
    int icon_type;
} GameCategory;

enum DRIVERS {INFERNO=0, MARCH33=1, NP9660=2};
enum EXECUTE {EBOOT_BIN=0, BOOT_BIN=1, EBOOT_OLD=2};
enum PSBUTTON_MODE {MENU=0, LIVEAREA=1, STANDARD=2};
enum SUSPEND_THREADS {SUSPEND_YES=0, SUSPEND_NO=1};
enum PLUGINS {PLUGINS_DEFAULT=0, PLUGINS_ENABLE=1, PLUGINS_DISABLE=2};
enum NONPDRM {NONPDRM_DEFAULT=0, NONPDRM_ENABLE=1, NONPDRM_DISABLE=2};
enum HIGH_MEMORY {HIGH_MEM_DEFAULT=0, HIGH_MEM_ENABLE=1, HIGH_MEM_DISABLE=2};
enum CPU_SPEED {CPU_DEFAULT=0, CPU_20_10=1, CPU_50_25=2, CPU_75_37=3, CPU_100_50=4, CPU_111_55, CPU_122_61, CPU_133_66,
                CPU_166_83, CPU_200_100, CPU_222_111, CPU_266_133, CPU_288_144, CPU_300_150, CPU_333_166};

typedef struct {
    DRIVERS driver;
    EXECUTE execute;
    bool customized;
    PSBUTTON_MODE ps_button_mode;
    SUSPEND_THREADS suspend_threads;
    PLUGINS plugins;
    NONPDRM nonpdrm;
    HIGH_MEMORY high_memory;
    CPU_SPEED cpu_speed;
	bool bilinear;
	bool gles1;
	bool skip_splash;
	bool compress_textures;
	bool fake_win_mode;
	bool debug_mode;
	bool debug_shaders;
	bool mem_extended;
	bool newlib_extended;
    bool video_support;
    bool has_net;
    bool squeeze_mem;
    bool disable_audio;
    bool uncached_mem;
    bool double_buffering;
} BootSettings;

struct PluginSetting {
    char plugin[256];
    bool enable;

    friend bool operator<(PluginSetting const& a, PluginSetting const& b)
    {
        return strcmp(a.plugin, b.plugin) < 0;
    }
} ;

#define FAVORITES 0
#define VITA_GAMES 1
#define SYSTEM_APPS 2
#define PSP_GAMES 3
#define PS1_GAMES 4
#define PS_MIMI_GAMES 5
#define PS_MOBILE_GAMES 6
#define NES_GAMES 7
#define SNES_GAMES 8
#define GB_GAMES 9
#define GBC_GAMES 10
#define GBA_GAMES 11
#define N64_GAMES 12
#define NEOGEO_GAMES 13
#define NEOGEO_CD_GAMES 14
#define NEOGEO_PC_GAMES 15
#define SEGA_SATURN_GAMES 16
#define GAME_GEAR_GAMES 17
#define MASTER_SYSTEM_GAMES 18
#define MEGA_DRIVE_GAMES 19
#define SEGA_32X_GAMES 20
#define SEGA_CD_GAMES 21
#define SEGA_DREAMCAST_GAMES 22
#define NEC_GAMES 23
#define ATARI_2600_GAMES 24
#define ATARI_5200_GAMES 25
#define ATARI_7800_GAMES 26
#define ATARI_LYNX_GAMES 27
#define AMIGA_GAMES 28
#define BANDAI_GAMES 29
#define C64_GAMES 30
#define MSX1_GAMES 31
#define MSX2_GAMES 32
#define T_GRAFX_GAMES 33
#define VECTREX_GAMES 34
#define GAW_GAMES 35
#define MAME_2000_GAMES 36
#define MAME_2003_GAMES 37
#define GMS_GAMES 38
#define SCUMMVM_GAMES 39
#define EASYRPG_GAMES 40
#define PORT_GAMES 41
#define ORIGINAL_GAMES 42
#define UTILITIES 43
#define EMULATORS 44
#define HOMEBREWS 45
#define CATEGORY 46

#define TOTAL_CATEGORY 46
#define TOTAL_ROM_CATEGORY 32

#define TYPE_BUBBLE 0
#define TYPE_ROM 1
#define TYPE_PSP_ISO 2
#define TYPE_EBOOT 3
#define TYPE_SCUMMVM 4
#define TYPE_GMS 5
#define TYPE_EASYRPG 6
#define TYPE_CATEGORY 126
#define TYPE_FOLDER 127

#define FOLDER_TYPE_ROOT 1
#define FOLDER_TYPE_SUBFOLDER 2
#define FOLDER_ROOT_ID 0

extern GameCategory game_categories[];
extern GameCategory* sorted_categories[];
extern std::map<std::string, GameCategory*> categoryMap;
extern GameCategory *current_category;
extern int games_to_scan;
extern int games_scanned;
extern Game game_scan_inprogress;
extern char scan_message[];
extern int ROM_CATEGORIES[];
extern char adernaline_launcher_boot_bin_path[];
extern char adernaline_launcher_config_bin_path[];
extern char adernaline_launcher_title_id[];
extern BootSettings default_boot_settings;
extern std::vector<PluginSetting> default_psp_plugin_settings;
extern std::vector<PluginSetting> default_ps1_plugin_settings;
extern std::vector<std::string> psp_iso_extensions;
extern std::vector<std::string> eboot_extensions;
extern std::vector<std::string> hidden_title_ids;
extern char pspemu_path[];
extern char gms_data_path[];
extern char pspemu_iso_path[];
extern char pspemu_eboot_path[];
extern char game_uninstalled;
extern std::vector<Game*> selected_games;
extern FtpClient *ftpclient;
extern int64_t bytes_transfered;
extern int64_t bytes_to_download;
extern bool download_error;
extern char download_error_message[512];
static SceUID load_images_thid = -1;
static SceUID scan_games_thid = -1;
static SceUID scan_games_category_thid = -1;
static SceUID load_image_thid = -1;
static SceUID delete_images_thid = -1;
static SceUID download_images_thid = -1;
static SceUID uninstall_game_thid = -1;
static SceUID download_game_thid = -1;
static SceUID get_cachestate_thid = -1;
static SceUID delete_cached_roms_thid = -1;

typedef struct LoadImagesParams {
  int category;
  int prev_page_num;
  int page_num;
  int games_per_page;
} LoadImagesParams;

typedef struct ScanGamesParams {
  const char* category;
  int type;
} ScanGamesParams;

typedef struct DeleteImagesParams {
    Folder *folder;
    int timeout;
};

namespace GAME {
    int GameComparator(const void *v1, const void *v2);
    int GameCategoryComparator(const void *v1, const void *v2);
    void Init();
    void Scan();
    std::vector<std::string> GetRomFiles(const std::string path);
    std::vector<std::string> GetGMSRomFiles(const std::string path);
    void ScanRetroCategory(sqlite3 *db, GameCategory *category);
    void ScanRetroGames(sqlite3 *db);
    int PopulateIsoGameInfo(Game *game, std::string rom, int game_index);
    void ScanAdrenalineIsoGames(sqlite3 *db);
    int PopulateEbootGameInfo(Game *game, std::string rom, int game_index);
    void ScanAdrenalineEbootGames(sqlite3 *db);
    void ScanScummVMGames(sqlite3 *db);
    void ScanGMSGames(sqlite3 *db);
    void ScanEasyRpgGames(sqlite3 *db);
    bool Launch(Game *game, BootSettings *settings = nullptr, char* retro_core = nullptr);
    void LoadGamesCache(sqlite3 *db);
    void LoadGameImages(int category, int prev_page, int page_num, int games_per_page);
    void LoadGameImage(Game *game);
    void StartLoadGameImageThread(int category, int game_num, int games_per_page);
    int LoadGameImageThread(SceSize args, LoadImagesParams *params);
    void Exit();
    int LoadImagesThread(SceSize args, LoadImagesParams *argp);
    int IncrementPage(int page, int num_of_pages);
    int DecrementPage(int page, int num_of_pages);
    void StartLoadImagesThread(int category, int prev_page_num, int page, int games_per_page);
    int ScanGamesThread(SceSize args, void *argp);
    void StartScanGamesThread();
    int DeleteGamesImagesThread(SceSize args, DeleteImagesParams *params);
    void StartDeleteGameImagesThread(GameCategory *category, int timeout);
    void SetMaxPage(GameCategory *category);
    Game* FindGame(GameCategory *category, Game *game);
    int FindGamePosition(GameCategory *category, Game *game);
    int RemoveGameFromCategory(GameCategory *category, Game *game);
    int RemoveGameFromFolder(Folder *folder, Game *game);
    void SortGames(GameCategory *category);
    void SortGames(Folder *folder);
    void RefreshGames(bool all_categories);
    const char* GetGameCategory(const char *id);
    GameCategory* GetRomCategoryByName(const char* category_name);
    bool IsRomCategory(int categoryId);
    bool IsRomExtension(std::string str, std::vector<std::string> &file_filters);
    std::string str_tolower(std::string s);
    void StartScanGamesCategoryThread(GameCategory *category);
    int ScanGamesCategoryThread(SceSize args, ScanGamesParams *params);
    bool IsMatchPrefixes(const char* id, std::vector<std::string> &prefixes);
    int IncrementCategory(int category_id, int num_of_ids);
    int DecrementCategory(int category_id, int num_of_ids);
    void DownloadThumbnail(sqlite3 *database, Game *game);
    void DownloadThumbnails(GameCategory *category);
    void StartDownloadThumbnailsThread(GameCategory *category);
    void FindGamesByPartialName(std::vector<GameCategory*> &categories, char* search_text, std::vector<Game> &games);
    void UninstallGame(Game *game);
    int UninstallGameThread(SceSize args, Game *game);
    void StartUninstallGameThread(Game *game);
    int DeleteApp(const char *titleid);
    Folder* FindFolder(GameCategory *category, int folder_id);
    void MoveGamesBetweenFolders(GameCategory *category, int src_id, int dest_id);
    int RemoveFolderFromCategory(GameCategory *category, int folder_id);
    void ClearSelection(GameCategory *category);
    std::vector<Game> GetSelectedGames(GameCategory *category);
    int GetSortedCategoryIndex(int category_id);
    int CategoryComparator(const void *v1, const void *v2);
    void SortCategories();
    void SortGameCategories();
    void HideCategories();
    void ShowAllCategories();
    char GetCacheState(Game *game);
    bool IsRemoteGame(Game *game);
    void DownloadGameToFtpCache(Game *game);
    void StartDownloadGameThread(Game *game);
    int DownloadGameThread(SceSize args, Game **game);
    void StartGetCacheStateThread();
    int GetCacheStateThread(SceSize args, void *argp);
    void MigratePSPCache();
    void DeleteCachedRom(Game *game);
    std::vector<std::string> GetFilesFromCueFile(char *path);
    void StartDeleteCachedRomsThread(GameCategory *category);
    int DeleteCachedRomsThread(SceSize args, ScanGamesParams *params);
    void DeleteCachedRoms(GameCategory *category);
    void LoadYoYoSettings(Game *game, BootSettings *settings);
    void SaveYoYoSettings(Game *game, BootSettings *settings);
    void ImportPspGamePlugins();
    void ImportPopsGamePlugins();
    bool GetPerGamePluginSettings(Game *game, std::vector<PluginSetting> &settings);
    void SyncPerGamePluginSettings(Game *game, std::vector<PluginSetting> &settings);
    void WritePerGamePluginSettings(Game *game, std::vector<PluginSetting> &settings);

    static int DownloadGameCallback(int64_t xfered, void* arg);
    static int LoadScePaf();
    static int UnloadScePaf();
}

#endif
