#ifndef LAUNCHER_DB_H
#define LAUNCHER_DB_H

#include "sqlite3.h"
#include "game.h"

#define CACHE_DB_FILE "ux0:data/SMLA00001/cache.db"
#define PER_GAME_SETTINGS_DB_FILE "ux0:data/SMLA00001/game_settings.db"
#define VITA_APP_DB_FILE "ur0:shell/db/app.db"
#define MAME_ROM_NAME_MAPPINGS_FILE "ux0:app/SMLA00001/thumbnails/mame_mapping.db"

#define GAMES_TABLE "games"
#define FAVORITES_TABLE "favorites"
#define FOLDERS_TABLE "folders"
#define PSP_GAME_SETTINGS_TABLE "psp_settings"
#define PSP_PLUGINS_SETTINGS_TABLE "psp_plugin_settings"
#define RETROROM_GAME_SETTINGS_TABLE "retrorom_settings"
#define APP_FOLDERS_TABLE "app_folders"
#define UPDATED_TABLE "updated"

#define COL_TITLE_ID "title_id"
#define COL_TYPE "type"
#define COL_TITLE "title"
#define COL_CATEGORY "category"
#define COL_ROM_PATH "rom_path"
#define COL_ID "id"
#define COL_FOLDER_ID "folder_id"
#define COL_ICON_PATH "icon_path"
#define COL_PLUGIN_PATH "plugin_path"
#define COL_PLUGIN_ENABLE "plugin_enable"
#define COL_UPDATED "updated"
#define COL_VALUE "value"

#define COL_DRIVERS                   "drivers"
#define COL_EXECUTE                   "execute"
#define COL_CUSTOMIZED                "customized"
#define COL_PSBUTTON_MODE             "ps_button_mode"
#define COL_SUSPEND_THREADS           "suspend_threads"
#define COL_PLUGINS                   "plugins"
#define COL_NONPDRM                   "nonpdrm"
#define COL_HIGH_MEMORY               "high_memory"
#define COL_CPU_SPEED                 "cpu_speed"

#define COL_RETRO_CORE                "retro_code"

namespace DB {
    bool TableExists(sqlite3 *db, char* table_name);
    bool TableColumnExists(sqlite3 *db, char* table_name, char* column_name);
    void GetVitaDbGames();
    int GetVitaDbGamesCount();
    void SetupDatabase(sqlite3 *database);
    void UpdateDatabase(sqlite3 *database);
    void InsertGame(sqlite3 *database, Game *game);
    void InsertFolder(sqlite3 *database, Folder *folder);
    void UpdateFolder(sqlite3 *database, Folder *folder);
    void GetFolders(sqlite3 *database, GameCategory *category);
    bool GameExists(sqlite3 *database, Game *game);
    int GetCachedGamesCount(sqlite3 *database);
    void GetCachedGames(sqlite3 *database);
    void DeleteGame(sqlite3 *database, Game *game);
    void DeleteFolder(sqlite3 *database, Folder *folder);
    void ResetGamesFolderId(sqlite3 *database, Folder *folder);
    void DeleteFavorite(sqlite3 *database, Game *game);
    void InsertFavorite(sqlite3 *database, Game *game);
    void GetFavorites(sqlite3 *database, GameCategory *category);
    void DeleteGamesByCategoryAndType(sqlite3 *database, const char* category, int type);
    void DeleteGamesByType(sqlite3 *database, int type);
    void UpdateFavoritesGameCategoryById(sqlite3 *database, Game *game);
    void UpdateFavoritesGameCategoryByRomPath(sqlite3 *database, Game *game);
    void UpdateGameTitle(sqlite3 *database, Game *game);
    void UpdateGame(sqlite3 *database, Game *game);
    void GetMaxTitleIdByType(sqlite3 *database, int type, char* max_title_id);
    bool FindMatchingThumbnail(char* db_name, std::vector<std::string> &tokens, char* thumbnail);
    bool FindMatchingThumbnail(sqlite3 *database, std::vector<std::string> &tokens, char* thumbnail);
    void SetupPerGameSettingsDatabase();
    void UpdateGameSettingsDatabase();
    void GetPspGameSettings(char* rom_path, BootSettings *settings);
    void GetRomCoreSettings(char* rom_path, char* core);
    void SavePspGameSettings(char* rom_path, BootSettings *settings);
    void SaveRomCoreSettings(char* rom_path, char* core);
    void GetPspPluginSettings(char* rom_path, std::vector<PluginSetting> &plugins);
    bool UpdatePspPluginSettings(sqlite3 *database, char* rom_path, const PluginSetting *settings);
    void SavePspPluginSettings(sqlite3 *database, char* rom_path, const PluginSetting *settings);
    void SavePspPluginSettings(char* rom_path, std::vector<PluginSetting> &plugins);
    void DeletePspPluginSettings(char* rom_path);
    void DeletePspPluginSettings(char* rom_path, PluginSetting *setting);
    bool GetMameRomName(sqlite3 *database, char* rom_name, char* name);
    void InsertVitaAppFolder(sqlite3 *database, char* title_id, int folder_id);
    int UpdateVitaAppFolder(sqlite3 *database, char* title_id, int folder_id);
    void DeleteVitaAppFolder(sqlite3 *database, int folder_id);
    void DeleteVitaAppFolderById(sqlite3 *database, char* title_id);
    void PerGameUpdated(sqlite3 *database);
}

#endif
