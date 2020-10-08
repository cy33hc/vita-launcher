#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "sqlite3.h"
#include "db.h"
#include "game.h"
#include "textures.h"

#define LAUNCHER_GAME_DB_FILE "ux0:data/SMLA00001/game.db"
#define VITA_APP_DB_FILE = "ur0:shell/db/app.db"

static sqlite3 *db;

static const char* tables[] = {
	"vita_games",
	"psp_games",
	"homebrews",
	"favorites",
	NULL
};

namespace DB {
    bool TableExists(sqlite3 *db, char* table_name)
    {
        sqlite3_stmt *res;
        std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, table_name, strlen(table_name), NULL);
        } else {
            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        }

        int step = sqlite3_step(res);
        sqlite3_finalize(res);
        if (step == SQLITE_ROW) {
            return true;
        }

        return false;
    }

    void GetVitaDbGames(GameCategory *category)
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open("ur0:shell/db/app.db", &db);
        std::string sql = "select titleId,val from tbl_appinfo where key=572932585 and titleID not like 'NPXS%'";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            std::string title = std::string((const char*)sqlite3_column_text(res, 1));
            std::replace( title.begin(), title.end(), '\n', ' ');
            sprintf(game.title, "%s", title.c_str());
            sprintf(game.category, "%s", GAME::GetGameCategory(game.id));
            game.tex = no_icon;
            game.type = TYPE_BUBBLE;
            category->games.push_back(game);
            game_scan_inprogress = game;
            step = sqlite3_step(res); 
        }
        sqlite3_finalize(res);
        sqlite3_close(db);
    }

    int GetVitaDbGamesCount()
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open("ur0:shell/db/app.db", &db);
        std::string sql = "select count(distinct(titleId)) from tbl_appinfo where key=572932585 and titleID not like 'NPXS%'";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        int count = 0;
        if (step == SQLITE_ROW)
        {
            count = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);
        sqlite3_close(db);

        return count;
    }
}
