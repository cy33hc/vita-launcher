#ifndef LAUNCHER_DB_H
#define LAUNCHER_DB_H

#include "sqlite3.h"
#include "game.h"

namespace DB {
    bool TableExists(sqlite3 *db, char* table_name);
    void GetVitaDbGames(GameCategory *category);
    int GetVitaDbGamesCount();
}

#endif
