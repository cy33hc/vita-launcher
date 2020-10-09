#ifndef LAUNCHER_DB_H
#define LAUNCHER_DB_H

#include "sqlite3.h"
#include "game.h"

#define CACHE_DB_FILE "ux0:data/SMLA00001/cache.db"
#define VITA_APP_DB_FILE "ur0:shell/db/app.db"

#define GAMES_TABLE "games"
#define FAVORITES_TABLE "favorites"

#define COL_TITLE_ID "title_id"
#define COL_TYPE "type"
#define COL_TITLE "title"
#define COL_CATEGORY "category"
#define COL_ROM_PATH "rom_path"

namespace DB {
    bool TableExists(sqlite3 *db, char* table_name);
    void GetVitaDbGames(GameCategory *category);
    int GetVitaDbGamesCount();
    void SetupDatabase(sqlite3 *database);
    void InsertGame(sqlite3 *database, Game *game);
    bool GameExists(sqlite3 *database, const char* rom_path);
    int GetCachedGamesCount(sqlite3 *database);
    void GetCachedGames(sqlite3 *database, GameCategory *category);
    void DeleteGame(sqlite3 *database, Game *game);
    void DeleteFavorite(sqlite3 *database, Game *game);
    void InsertFavorite(sqlite3 *database, Game *game);
    void GetFavorites(sqlite3 *database, GameCategory *category);
}

#endif
