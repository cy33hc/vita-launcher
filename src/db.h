#ifndef LAUNCHER_DB_H
#define LAUNCHER_DB_H

#include "sqlite3.h"
#include "game.h"

#define CACHE_DB_FILE "ux0:data/SMLA00001/cache.db"
#define PER_GAME_SETTINGS_DB_FILE "ux0:data/SMLA00001/game_settings.db"
#define VITA_APP_DB_FILE "ur0:shell/db/app.db"

#define GAMES_TABLE "games"
#define FAVORITES_TABLE "favorites"
#define PSP_GAME_SETTINGS_TABLE "psp_settings"
#define RETROROM_GAME_SETTINGS_TABLE "retrorom_settings"

#define COL_TITLE_ID "title_id"
#define COL_TYPE "type"
#define COL_TITLE "title"
#define COL_CATEGORY "category"
#define COL_ROM_PATH "rom_path"

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
    void GetVitaDbGames(GameCategory *category);
    int GetVitaDbGamesCount();
    void SetupDatabase(sqlite3 *database);
    void InsertGame(sqlite3 *database, Game *game);
    bool GameExists(sqlite3 *database, Game *game);
    int GetCachedGamesCount(sqlite3 *database);
    void GetCachedGames(sqlite3 *database);
    void DeleteGame(sqlite3 *database, Game *game);
    void DeleteFavorite(sqlite3 *database, Game *game);
    void InsertFavorite(sqlite3 *database, Game *game);
    void GetFavorites(sqlite3 *database, GameCategory *category);
    void DeleteGamesByCategoryAndType(sqlite3 *database, const char* category, int type);
    void DeleteGamesByType(sqlite3 *database, int type);
    void UpdateFavoritesGameCategoryById(sqlite3 *database, Game *game);
    void UpdateFavoritesGameCategoryByRomPath(sqlite3 *database, Game *game);
    void UpdateGameCategory(sqlite3 *database, Game *game);
    void UpdateGameTitle(sqlite3 *database, Game *game);
    void GetMaxTitleIdByType(sqlite3 *database, int type, char* max_title_id);
    void FindMatchingThumbnail(char* db_name, std::vector<std::string> &tokens, char* thumbnail);
    void FindMatchingThumbnail(sqlite3 *database, std::vector<std::string> &tokens, char* thumbnail);
    void SetupPerGameSettingsDatabase();
    void GetPspGameSettings(char* rom_path, BootSettings *settings);
    void GetRomCoreSettings(char* rom_path, char* core);
    void SavePspGameSettings(char* rom_path, BootSettings *settings);
    void SaveRomCoreSettings(char* rom_path, char* core);
}

#endif
