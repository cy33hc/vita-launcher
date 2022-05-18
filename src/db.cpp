#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "sqlite3.h"
#include "db.h"
#include "game.h"
#include "textures.h"

namespace DB {
    bool TableExists(sqlite3 *database, char* table_name)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        bool found = false;
        std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, table_name, strlen(table_name), NULL);

            int step = sqlite3_step(res);
            sqlite3_finalize(res);
            if (step == SQLITE_ROW) {
                found = true;
            }
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
        return found;
    }

    bool TableColumnExists(sqlite3 *database, char* table_name, char* column_name)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        bool found = false;
        std::string sql = std::string("PRAGMA table_info(") + table_name + ")";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        
        if (rc == SQLITE_OK) {
            char db_col_name[256];
            while (sqlite3_step(res) == SQLITE_ROW)
            {
                sprintf(db_col_name, "%s", sqlite3_column_text(res, 1));
                if (strcmp(db_col_name, column_name) == 0)
                {
                    found = true;
                    break;
                }
            }
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
        return found;
    }

    void GetVitaDbGames()
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open(VITA_APP_DB_FILE, &db);
        std::string sql = std::string("select titleId,val,folder_id ") +
            "from tbl_appinfo left join app_folders on tbl_appinfo.titleId = app_folders.id " +
            "where tbl_appinfo.key=572932585 and (tbl_appinfo.titleID not like 'NPXS%' or tbl_appinfo.titleId = 'NPXS10000' or tbl_appinfo.titleId = 'NPXS10001' or tbl_appinfo.titleId = 'NPXS10002' or tbl_appinfo.titleId = 'NPXS10003' or tbl_appinfo.titleId = 'NPXS10004' or tbl_appinfo.titleId = 'NPXS10006' or tbl_appinfo.titleId = 'NPXS10008' or tbl_appinfo.titleId = 'NPXS10009' or tbl_appinfo.titleId = 'NPXS10010' or tbl_appinfo.titleId = 'NPXS10014' or tbl_appinfo.titleId = 'NPXS10015' or tbl_appinfo.titleId = 'NPXS10026' or tbl_appinfo.titleId = 'NPXS10031' or tbl_appinfo.titleId = 'NPXS10072' or tbl_appinfo.titleId = 'NPXS10091' or tbl_appinfo.titleId = 'NPXS10098' or tbl_appinfo.titleId = 'NPXS10094')";
        bool app_folder_exists = TableExists(db, APP_FOLDERS_TABLE);
        if (!app_folder_exists)
        {
            sql = "select titleId,val from tbl_appinfo where key=572932585 and (tbl_appinfo.titleID not like 'NPXS%' or tbl_appinfo.titleId = 'NPXS10000' or tbl_appinfo.titleId = 'NPXS10001' or tbl_appinfo.titleId = 'NPXS10002' or tbl_appinfo.titleId = 'NPXS10003' or tbl_appinfo.titleId = 'NPXS10004' or tbl_appinfo.titleId = 'NPXS10006' or tbl_appinfo.titleId = 'NPXS10008' or tbl_appinfo.titleId = 'NPXS10009' or tbl_appinfo.titleId = 'NPXS10010' or tbl_appinfo.titleId = 'NPXS10014' or tbl_appinfo.titleId = 'NPXS10015' or tbl_appinfo.titleId = 'NPXS10026' or tbl_appinfo.titleId = 'NPXS10031' or tbl_appinfo.titleId = 'NPXS10072' or tbl_appinfo.titleId = 'NPXS10091' or tbl_appinfo.titleId = 'NPXS10098' or tbl_appinfo.titleId = 'NPXS10094')";
        }
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            if (!GAME::IsMatchPrefixes(game.id, hidden_title_ids) || strcmp(game.id, "PSPEMUCFW")==0)
            {
                std::string title = std::string((const char*)sqlite3_column_text(res, 1));
                std::replace( title.begin(), title.end(), '\n', ' ');
                sprintf(game.category, "%s", GAME::GetGameCategory(game.id));
                sprintf(game.title, "%s", title.c_str());
                sprintf(game.rom_path, "%s", "");
                if (app_folder_exists)
                {
                    game.folder_id = sqlite3_column_int(res, 2);
                }
                else
                {
                    game.folder_id = 0;
                }
                game.tex = no_icon;
                game.type = TYPE_BUBBLE;

                Folder *folder = GAME::FindFolder(categoryMap[game.category], game.folder_id);
                if (folder != nullptr)
                {
                    folder->games.push_back(game);
                }
                else
                {
                    game.folder_id = 0;
                    categoryMap[game.category]->current_folder->games.push_back(game);
                }

                game_scan_inprogress = game;
            }
            games_scanned++;
            step = sqlite3_step(res); 
        }
        sqlite3_finalize(res);
        sqlite3_close(db);
    }

    int GetVitaDbGamesCount()
    {
        sqlite3 *db;
        sqlite3_stmt *res;

        int rc = sqlite3_open(VITA_APP_DB_FILE, &db);
        std::string sql = "select count(distinct(titleId)) from tbl_appinfo where key=572932585 and (tbl_appinfo.titleID not like 'NPXS%' or tbl_appinfo.titleId = 'NPXS10000' or tbl_appinfo.titleId = 'NPXS10001' or tbl_appinfo.titleId = 'NPXS10002' or tbl_appinfo.titleId = 'NPXS10003' or tbl_appinfo.titleId = 'NPXS10004' or tbl_appinfo.titleId = 'NPXS10006' or tbl_appinfo.titleId = 'NPXS10008' or tbl_appinfo.titleId = 'NPXS10009' or tbl_appinfo.titleId = 'NPXS10010' or tbl_appinfo.titleId = 'NPXS10014' or tbl_appinfo.titleId = 'NPXS10015' or tbl_appinfo.titleId = 'NPXS10026' or tbl_appinfo.titleId = 'NPXS10031' or tbl_appinfo.titleId = 'NPXS10072' or tbl_appinfo.titleId = 'NPXS10091' or tbl_appinfo.titleId = 'NPXS10098' or tbl_appinfo.titleId = 'NPXS10094')";
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

    void SetupDatabase(sqlite3 *database)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        if (!TableExists(db, GAMES_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + GAMES_TABLE + "(" +
                COL_TITLE_ID + " TEXT," +
                COL_TITLE + " TEXT," +
                COL_TYPE + " INTEGER," +
                COL_CATEGORY + " TEXT," +
                COL_ROM_PATH + " TEXT," +
                COL_FOLDER_ID + " INTEGER DEFAULT 0)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX games_index ON ") + GAMES_TABLE + "(" + 
                COL_TITLE + "," + COL_TYPE + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX games_index_folder_cat ON ") + GAMES_TABLE + "(" + 
                COL_FOLDER_ID + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX games_index_rom_path ON ") + GAMES_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }        

        if (!TableExists(db, FAVORITES_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + FAVORITES_TABLE + "(" +
                COL_TITLE_ID + " TEXT," +
                COL_TITLE + " TEXT," +
                COL_TYPE + " INTEGER," +
                COL_CATEGORY + " TEXT," +
                COL_ROM_PATH + " TEXT," +
                COL_FOLDER_ID + " INTEGER DEFAULT 0)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX favorites_index ON ") + FAVORITES_TABLE + "(" + 
                COL_TITLE + "," + COL_TYPE + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX favorites_index_folder_cat ON ") + FAVORITES_TABLE + "(" + 
                COL_FOLDER_ID + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX favorites_index_rom_path ON ") + FAVORITES_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, FOLDERS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + FOLDERS_TABLE + "(" +
                COL_ID + " INTEGER," +
                COL_TITLE + " TEXT," +
                COL_CATEGORY + " TEXT," +
                COL_ICON_PATH + " TEXT," +
                "PRIMARY KEY(" + COL_ID +"))";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        sqlite3 *vita_app_db;
        sqlite3_open(VITA_APP_DB_FILE, &vita_app_db);
        if (!TableExists(vita_app_db, APP_FOLDERS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + APP_FOLDERS_TABLE + "(" +
                COL_ID + " TEXT," +
                COL_FOLDER_ID + " INTEGER," +
                "PRIMARY KEY(" + COL_ID +"))";
            sqlite3_exec(vita_app_db, sql.c_str(), NULL, NULL, NULL);
        }
        sqlite3_close(vita_app_db);
    }

    void UpdateDatabase(sqlite3 *database)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        if (!TableColumnExists(db, GAMES_TABLE, COL_FOLDER_ID))
        {
            std::string sql = std::string("ALTER TABLE ") + GAMES_TABLE +
                " ADD " + COL_FOLDER_ID + " INTEGER DEFAULT 0";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX games_index_folder_cat ON ") + GAMES_TABLE + "(" + 
                COL_FOLDER_ID + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX games_index_rom_path ON ") + GAMES_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }
        
        if (!TableColumnExists(db, FAVORITES_TABLE, COL_FOLDER_ID))
        {
            std::string sql = std::string("ALTER TABLE ") + FAVORITES_TABLE +
                " ADD " + COL_FOLDER_ID + " INTEGER DEFAULT 0";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX favorites_index_folder_cat ON ") + FAVORITES_TABLE + "(" + 
                COL_FOLDER_ID + "," + COL_CATEGORY + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX favorites_index_rom_path ON ") + FAVORITES_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, FOLDERS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + FOLDERS_TABLE + "(" +
                COL_ID + " INTEGER," +
                COL_TITLE + " TEXT," +
                COL_CATEGORY + " TEXT," +
                COL_ICON_PATH + " TEXT," +
                "PRIMARY KEY(" + COL_ID +"))";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        sqlite3 *vita_app_db;
        sqlite3_open(VITA_APP_DB_FILE, &vita_app_db);
        if (!TableExists(vita_app_db, APP_FOLDERS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + APP_FOLDERS_TABLE + "(" +
                COL_ID + " TEXT," +
                COL_FOLDER_ID + " INTEGER," +
                "PRIMARY KEY(" + COL_ID +"))";
            sqlite3_exec(vita_app_db, sql.c_str(), NULL, NULL, NULL);
        }
        sqlite3_close(vita_app_db);
    }

    void InsertFavorite(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("INSERT INTO ") + FAVORITES_TABLE + "(" + COL_TITLE_ID + "," +
            COL_TITLE + "," + COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + ") VALUES (?, ?, ?, ?, ?)";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->id, strlen(game->id), NULL);
            sqlite3_bind_text(res, 2, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 3, game->type);
            sqlite3_bind_text(res, 4, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 5, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteFavorite(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + FAVORITES_TABLE + " WHERE " + COL_TITLE + "=? AND " +
            COL_TYPE + "=? AND " + COL_CATEGORY + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 2, game->type);
            sqlite3_bind_text(res, 3, game->category, strlen(game->category), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void GetFavorites(sqlite3 *database, GameCategory *category)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_TITLE_ID + "," + COL_TITLE + "," +
            COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + " FROM " + FAVORITES_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            sprintf(game.title, "%s", sqlite3_column_text(res, 1));
            game.type = sqlite3_column_int(res, 2);
            sprintf(game.category, "%s", sqlite3_column_text(res, 3));
            sprintf(game.rom_path, "%s", sqlite3_column_text(res, 4));
            game.tex = no_icon;
            games_scanned++;
            game_scan_inprogress = game;
            category->current_folder->games.push_back(game);
            step = sqlite3_step(res);
        }
        sqlite3_finalize(res);
        
        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void InsertGame(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("INSERT INTO ") + GAMES_TABLE + "(" + COL_TITLE_ID + "," + 
            COL_TITLE + "," + COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + ") VALUES (?, ?, ?, ?, ?)";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->id, strlen(game->id), NULL);
            sqlite3_bind_text(res, 2, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 3, game->type);
            sqlite3_bind_text(res, 4, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 5, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   bool GameExists(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql;
        if (game->type != TYPE_BUBBLE)
        {
            sql = std::string("SELECT * FROM ") + GAMES_TABLE + " WHERE " + 
                COL_ROM_PATH + "=? AND " + COL_TYPE + "=?";
        }
        else
        {
            sql = std::string("SELECT * FROM ") + GAMES_TABLE + " WHERE " + 
                COL_TITLE_ID + "=? AND " + COL_TYPE + "=?";
        }
        
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        bool found = false;
        if (rc == SQLITE_OK) {
            if (game->type != TYPE_BUBBLE)
            {
                sqlite3_bind_text(res, 1, game->rom_path, strlen(game->rom_path), NULL);
            }
            else
            {
                sqlite3_bind_text(res, 1, game->id, strlen(game->id), NULL);
            }
            
            sqlite3_bind_int(res, 2, game->type);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                found = true;
            }
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return found;
    }

    void InsertFolder(sqlite3 *database, Folder *folder)
    {
        sqlite3 *db = database;

        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT MAX(") + COL_ID + ") FROM " + FOLDERS_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        int max_id = 0;
        if (rc == SQLITE_OK) {
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                max_id = sqlite3_column_int(res, 0);
            }
            sqlite3_finalize(res);
        }
        folder->id = max_id + 1;

        sql = std::string("INSERT INTO ") + FOLDERS_TABLE + "(" + COL_ID + "," + 
            COL_TITLE + "," + COL_CATEGORY + "," + COL_ICON_PATH + ") VALUES (?, ?, ?, ?)";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, folder->id);
            sqlite3_bind_text(res, 2, folder->title, strlen(folder->title), NULL);
            sqlite3_bind_text(res, 3, folder->category, strlen(folder->category), NULL);
            sqlite3_bind_text(res, 4, folder->icon_path, strlen(folder->icon_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void UpdateFolder(sqlite3 *database, Folder *folder)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + FOLDERS_TABLE + " SET " + 
            COL_TITLE + "=?, " +
            COL_ICON_PATH + "=? " +
            " WHERE " + COL_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, folder->title, strlen(folder->title), NULL);
            sqlite3_bind_text(res, 2, folder->icon_path, strlen(folder->icon_path), NULL);
            sqlite3_bind_int(res, 3, folder->id);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    int GetCachedGamesCount(sqlite3 *database)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("select count(1) from ") + GAMES_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        int count = 0;
        if (step == SQLITE_ROW)
        {
            count = sqlite3_column_int(res, 0);
        }
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return count;
    }

    void GetCachedGames(sqlite3 *database)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_TITLE_ID + "," + COL_TITLE + "," +
            COL_TYPE + "," + COL_CATEGORY + "," + COL_ROM_PATH + "," + COL_FOLDER_ID + " FROM " + GAMES_TABLE;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        int step = sqlite3_step(res);
        while (step == SQLITE_ROW)
        {
            Game game;
            sprintf(game.id, "%s", sqlite3_column_text(res, 0));
            sprintf(game.title, "%s", sqlite3_column_text(res, 1));
            game.type = sqlite3_column_int(res, 2);
            sprintf(game.category, "%s", sqlite3_column_text(res, 3));
            sprintf(game.rom_path, "%s", sqlite3_column_text(res, 4));
            game.folder_id = sqlite3_column_int(res, 5);
            game.tex = no_icon;
            games_scanned++;
            game_scan_inprogress = game;
            Folder *folder = GAME::FindFolder(categoryMap[game.category], game.folder_id);
            if (folder != nullptr)
            {
                folder->games.push_back(game);
            }
            else
            {
                game.folder_id = 0;
                categoryMap[game.category]->current_folder->games.push_back(game);
            }
            
            step = sqlite3_step(res);
        }
        sqlite3_finalize(res);
        
        for (int i=0; i<TOTAL_CATEGORY; i++)
        {
            GameCategory *category = &game_categories[i];
            for (int j=1; j<category->folders.size(); j++)
            {
                Game game;
                sprintf(game.id, "%d", category->folders[j].id);
                sprintf(game.title, "%s", category->folders[j].title);
                sprintf(game.category, "%s", category->folders[j].category);
                game.type = TYPE_FOLDER;
                game.folder_id = category->folders[j].id;
                game.favorite = false;
                game.tex = no_icon;
                category->folders[0].games.insert(category->folders[0].games.begin(), game);
            }
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void GetFolders(sqlite3 *database, GameCategory *category)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_ID + "," + COL_TITLE + "," +
            COL_CATEGORY + "," + COL_ICON_PATH + " FROM " + FOLDERS_TABLE +
            " WHERE " + COL_CATEGORY + "=? ORDER BY " + COL_TITLE;

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, category->category, strlen(category->category), NULL);
            while (sqlite3_step(res) == SQLITE_ROW)
            {
                Folder folder;
                folder.id = sqlite3_column_int(res, 0);
                sprintf(folder.title, "%s", sqlite3_column_text(res, 1));
                sprintf(folder.category, "%s", sqlite3_column_text(res, 2));
                sprintf(folder.icon_path, "%s", sqlite3_column_text(res, 3));
                folder.max_page = 1;
                folder.page_num = 1;
                folder.type = FOLDER_TYPE_SUBFOLDER;
                category->folders.push_back(folder);
            }
        }
        category->current_folder = &category->folders[0];
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteGame(sqlite3 *database, Game *game)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + GAMES_TABLE + " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteFolder(sqlite3 *database, Folder *folder)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + FOLDERS_TABLE + " WHERE " + COL_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, folder->id);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        ResetGamesFolderId(db, folder);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void ResetGamesFolderId(sqlite3 *database, Folder *folder)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + GAMES_TABLE + " SET " + COL_FOLDER_ID + "=0" +
            " WHERE " + COL_FOLDER_ID + "=? AND " + COL_CATEGORY + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
    
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, folder->id);
            sqlite3_bind_text(res, 2, folder->category, strlen(folder->category), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteGamesByCategoryAndType(sqlite3 *database, const char* category, int type)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + GAMES_TABLE + " WHERE " + COL_TYPE + "=? AND " + COL_CATEGORY + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, type);
            sqlite3_bind_text(res, 2, category, strlen(category), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void DeleteGamesByType(sqlite3 *database, int type)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + GAMES_TABLE + " WHERE " + COL_TYPE + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, type);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void UpdateFavoritesGameCategoryById(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + FAVORITES_TABLE + " SET " + COL_CATEGORY + "=? WHERE " + COL_TITLE_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 2, game->id, strlen(game->id), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void UpdateFavoritesGameCategoryByRomPath(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + FAVORITES_TABLE + " SET " + COL_CATEGORY + "=? WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 2, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void UpdateGame(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + GAMES_TABLE + " SET " + 
            COL_CATEGORY + "=?, " +
            COL_TITLE + "=?, " +
            COL_FOLDER_ID + "=? " +
            " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->category, strlen(game->category), NULL);
            sqlite3_bind_text(res, 2, game->title, strlen(game->title), NULL);
            sqlite3_bind_int(res, 3, game->folder_id);
            sqlite3_bind_text(res, 4, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

   void UpdateGameTitle(sqlite3 *database, Game *game)
   {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + GAMES_TABLE + " SET " + COL_TITLE + "=? WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, game->title, strlen(game->title), NULL);
            sqlite3_bind_text(res, 2, game->rom_path, strlen(game->rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
   }

    void GetMaxTitleIdByType(sqlite3 *database, int type, char* max_title_id)
    {
        sqlite3 *db = database;
        if (db == nullptr)
        {
            sqlite3_open(CACHE_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("select max(title_id) from games where type=?");
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, type);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                sprintf(max_title_id, "%s", sqlite3_column_text(res, 0));
            }
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    bool FindMatchingThumbnail(char* db_name, std::vector<std::string> &tokens, char* thumbnail)
    {
        char db_path[64];
        sprintf(db_path, "ux0:app/SMLA00001/thumbnails/%s.db", db_name);
        sqlite3 *db;
        sqlite3_open(db_path, &db);
        bool found = FindMatchingThumbnail(db, tokens, thumbnail);
        sqlite3_close(db);
        return found;
    }

    bool FindMatchingThumbnail(sqlite3 *database, std::vector<std::string> &tokens, char* thumbnail)
    {
        sqlite3 *db = database;
        bool found = false;
        int tokens_to_try = tokens.size();

        sqlite3_stmt *res;
        while (!found && tokens_to_try>0)
        {
            std::string sql = std::string("select filename from thumbnails where ");
            for (int i=0; i<tokens_to_try; i++)
            {
                if (i!=0)
                {
                    sql += " and ";
                }
                if (tokens[i].compare("1") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%I%')";
                else if (tokens[i].compare("2") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%II%')";
                else if (tokens[i].compare("3") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%III%')";
                else if (tokens[i].compare("4") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%IV%')";
                else if (tokens[i].compare("5") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%V%')";
                else if (tokens[i].compare("6") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%VI%')";
                else if (tokens[i].compare("7") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%VII%')";
                else if (tokens[i].compare("8") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%VIII%')";
                else if (tokens[i].compare("I") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%1%')";
                else if (tokens[i].compare("II") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%2%')";
                else if (tokens[i].compare("III") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%3%')";
                else if (tokens[i].compare("IV") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%4%')";
                else if (tokens[i].compare("V") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%5%')";
                else if (tokens[i].compare("VI") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%6%')";
                else if (tokens[i].compare("VII") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%7%')";
                else if (tokens[i].compare("VIII") == 0)
                    sql += "(filename like '%" + tokens[i] + "%' or filename like '%8%')";
                else
                    sql += "filename like '%" + tokens[i] + "%'";
            }
            sql += " order by length(filename) asc";
            int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

            if (rc == SQLITE_OK) {
                int step = sqlite3_step(res);
                if (step == SQLITE_ROW)
                {
                    sprintf(thumbnail, "%s", sqlite3_column_text(res, 0));
                    found = true;
                }
                sqlite3_finalize(res);
            }
            --tokens_to_try;
        }

        return found;
    }
    
    void SetupPerGameSettingsDatabase()
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        if (!TableExists(db, PSP_GAME_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + PSP_GAME_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_DRIVERS + " INTEGER," +
                COL_EXECUTE + " INTEGER," +
                COL_CUSTOMIZED + " INTEGER," +
                COL_PSBUTTON_MODE + " INTEGER," +
                COL_SUSPEND_THREADS + " INTEGER," +
                COL_PLUGINS + " INTEGER," +
                COL_NONPDRM + " INTEGER," +
                COL_HIGH_MEMORY + " INTEGER," +
                COL_CPU_SPEED + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX psp_games_settings_index ON ") + PSP_GAME_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, PSP_PLUGINS_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + PSP_PLUGINS_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_PLUGIN_PATH + " TEXT," +
                COL_PLUGIN_ENABLE + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX psp_plugin_settings_index ON ") + PSP_PLUGINS_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, RETROROM_GAME_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + RETROROM_GAME_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_RETRO_CORE + " TEXT)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX retro_games_settings_index ON ") + RETROROM_GAME_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, UPDATED_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + UPDATED_TABLE + "(" +
                COL_UPDATED + " TEXT," +
                COL_VALUE + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX updated_table_index ON ") + UPDATED_TABLE + "(" + 
                COL_UPDATED + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sqlite3_stmt *res;
            sql = std::string("INSERT INTO ") + UPDATED_TABLE + 
                "(" + COL_UPDATED + ", " + COL_VALUE + ") VALUES (?,?)";
            int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

            if (rc == SQLITE_OK)
            {
                sqlite3_bind_text(res, 1, COL_UPDATED, strlen(COL_UPDATED), NULL);
                sqlite3_bind_int(res, 2, 0);
                sqlite3_step(res);
                sqlite3_finalize(res);
            }
        }

        sqlite3_close(db);
    }

    void UpdateGameSettingsDatabase()
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        if (!TableExists(db, PSP_GAME_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + PSP_GAME_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_DRIVERS + " INTEGER," +
                COL_EXECUTE + " INTEGER," +
                COL_CUSTOMIZED + " INTEGER," +
                COL_PSBUTTON_MODE + " INTEGER," +
                COL_SUSPEND_THREADS + " INTEGER," +
                COL_PLUGINS + " INTEGER," +
                COL_NONPDRM + " INTEGER," +
                COL_HIGH_MEMORY + " INTEGER," +
                COL_CPU_SPEED + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX psp_games_settings_index ON ") + PSP_GAME_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, PSP_PLUGINS_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + PSP_PLUGINS_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_PLUGIN_PATH + " TEXT," +
                COL_PLUGIN_ENABLE + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX psp_plugin_settings_index ON ") + PSP_PLUGINS_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, RETROROM_GAME_SETTINGS_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + RETROROM_GAME_SETTINGS_TABLE + "(" +
                COL_ROM_PATH + " TEXT," +
                COL_RETRO_CORE + " TEXT)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX retro_games_settings_index ON ") + RETROROM_GAME_SETTINGS_TABLE + "(" + 
                COL_ROM_PATH + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
        }

        if (!TableExists(db, UPDATED_TABLE))
        {
            std::string sql = std::string("CREATE TABLE ") + UPDATED_TABLE + "(" +
                COL_UPDATED + " TEXT," +
                COL_VALUE + " INTEGER)";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sql = std::string("CREATE INDEX updated_table_index ON ") + UPDATED_TABLE + "(" + 
                COL_UPDATED + ")";
            sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

            sqlite3_stmt *res;
            sql = std::string("INSERT INTO ") + UPDATED_TABLE + 
                "(" + COL_UPDATED + ", " + COL_VALUE + ") VALUES (?,?)";
            int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

            if (rc == SQLITE_OK)
            {
                sqlite3_bind_text(res, 1, COL_UPDATED, strlen(COL_UPDATED), NULL);
                sqlite3_bind_int(res, 2, 0);
                sqlite3_step(res);
                sqlite3_finalize(res);
            }
        }

        sqlite3_close(db);

    }

    void GetPspGameSettings(char* rom_path, BootSettings *settings)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_DRIVERS + "," + COL_EXECUTE + "," + 
            COL_CUSTOMIZED + "," + COL_PSBUTTON_MODE + "," + COL_SUSPEND_THREADS + "," + COL_PLUGINS + "," + 
            COL_NONPDRM + "," + COL_HIGH_MEMORY + "," + COL_CPU_SPEED + " FROM " + PSP_GAME_SETTINGS_TABLE +
            " WHERE " + COL_ROM_PATH + "=?";

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                settings->driver = sqlite3_column_int(res, 0);
                settings->execute = sqlite3_column_int(res, 1);
                settings->customized = sqlite3_column_int(res, 2);
                settings->ps_button_mode = sqlite3_column_int(res, 3);
                settings->suspend_threads = sqlite3_column_int(res, 4);
                settings->plugins = sqlite3_column_int(res, 5);
                settings->nonpdrm = sqlite3_column_int(res, 6);
                settings->high_memory = sqlite3_column_int(res, 7);
                settings->cpu_speed = sqlite3_column_int(res, 8);
            }
            sqlite3_finalize(res);
        }

        sqlite3_close(db);
    }

    void GetRomCoreSettings(char* rom_path, char* core)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_RETRO_CORE + " FROM " + 
            RETROROM_GAME_SETTINGS_TABLE + " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                sprintf(core, "%s", sqlite3_column_text(res, 0));
            }
            sqlite3_finalize(res);
        }

        sqlite3_close(db);
    }

    void SavePspGameSettings(char* rom_path, BootSettings *settings)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + PSP_GAME_SETTINGS_TABLE + " SET " + 
                COL_DRIVERS + "=?, " + COL_EXECUTE + "=?, " + COL_CUSTOMIZED + "=?, " +
                COL_PSBUTTON_MODE + "=?, " + COL_SUSPEND_THREADS + "=?, " + COL_PLUGINS + "=?, " + 
                COL_NONPDRM + "=?, " + COL_HIGH_MEMORY + "=?, " + COL_CPU_SPEED + "=? " + 
                "WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, settings->driver);
            sqlite3_bind_int(res, 2, settings->execute);
            sqlite3_bind_int(res, 3, settings->customized);
            sqlite3_bind_int(res, 4, settings->ps_button_mode);
            sqlite3_bind_int(res, 5, settings->suspend_threads);
            sqlite3_bind_int(res, 6, settings->plugins);
            sqlite3_bind_int(res, 7, settings->nonpdrm);
            sqlite3_bind_int(res, 8, settings->high_memory);
            sqlite3_bind_int(res, 9, settings->cpu_speed);
            sqlite3_bind_text(res, 10, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            int updated = sqlite3_changes(db);
            sqlite3_finalize(res);

            if (updated == 0)
            {
                sql = std::string("INSERT INTO ") + PSP_GAME_SETTINGS_TABLE + "(" + COL_ROM_PATH + "," + 
                    COL_DRIVERS + ", " + COL_EXECUTE + ", " + COL_CUSTOMIZED + ", " +
                    COL_PSBUTTON_MODE + ", " + COL_SUSPEND_THREADS + ", " + COL_PLUGINS + ", " + 
                    COL_NONPDRM + ", " + COL_HIGH_MEMORY + ", " + COL_CPU_SPEED + ") " + 
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
            
                if (rc == SQLITE_OK)
                {
                    sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
                    sqlite3_bind_int(res, 2, settings->driver);
                    sqlite3_bind_int(res, 3, settings->execute);
                    sqlite3_bind_int(res, 4, settings->customized);
                    sqlite3_bind_int(res, 5, settings->ps_button_mode);
                    sqlite3_bind_int(res, 6, settings->suspend_threads);
                    sqlite3_bind_int(res, 7, settings->plugins);
                    sqlite3_bind_int(res, 8, settings->nonpdrm);
                    sqlite3_bind_int(res, 9, settings->high_memory);
                    sqlite3_bind_int(res, 10, settings->cpu_speed);
                    step = sqlite3_step(res);
                    sqlite3_finalize(res);
                }
            }
        }

        PerGameUpdated(db);

        sqlite3_close(db);
    }

    void SaveRomCoreSettings(char* rom_path, char* core)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("UPDATE ") + RETROROM_GAME_SETTINGS_TABLE + " SET " + 
            COL_RETRO_CORE + "=? WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, core, strlen(core), NULL);
            sqlite3_bind_text(res, 2, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            int updated = sqlite3_changes(db);
            sqlite3_finalize(res);

            if (updated == 0)
            {
                sql = std::string("INSERT INTO ") + RETROROM_GAME_SETTINGS_TABLE + "(" +
                    COL_ROM_PATH + "," + COL_RETRO_CORE + ") VALUES (?, ?)";
                rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
            
                if (rc == SQLITE_OK)
                {
                    sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
                    sqlite3_bind_text(res, 2, core, strlen(core), NULL);
                    step = sqlite3_step(res);
                    sqlite3_finalize(res);
                }
            }
        }
        PerGameUpdated(db);
        sqlite3_close(db);
    }

    bool GetMameRomName(sqlite3 *database, char* rom_name, char* name)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(MAME_ROM_NAME_MAPPINGS_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT name FROM mappings WHERE rom_name=?");
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        bool found = false;
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, rom_name, strlen(rom_name), NULL);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                strlcpy(name, sqlite3_column_text(res, 0), 128);
                found = true;
            }
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return found;
    }

    void InsertVitaAppFolder(sqlite3 *database, char* title_id, int folder_id)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(VITA_APP_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("INSERT INTO ") + APP_FOLDERS_TABLE + 
            "(" + COL_ID + ", " + COL_FOLDER_ID + ") VALUES (?,?)";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, title_id, strlen(title_id), NULL);
            sqlite3_bind_int(res, 2, folder_id);
            sqlite3_step(res);
        }
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    int UpdateVitaAppFolder(sqlite3 *database, char* title_id, int folder_id)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(VITA_APP_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        int count = 0;
        std::string sql = std::string("UPDATE ") + APP_FOLDERS_TABLE + " SET " +
            COL_FOLDER_ID + "=? WHERE " + COL_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(res, 1, folder_id);
            sqlite3_bind_text(res, 2, title_id, strlen(title_id), NULL);
            rc = sqlite3_step(res);

            if (rc == SQLITE_DONE)
            {
                count = sqlite3_changes(db);
            }
        }
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return count;
    }

    void DeleteVitaAppFolder(sqlite3 *database, int folder_id)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(VITA_APP_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + APP_FOLDERS_TABLE +
            " WHERE " + COL_FOLDER_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(res, 1, folder_id);
            sqlite3_step(res);
        }
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void DeleteVitaAppFolderById(sqlite3 *database, char* title_id)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(VITA_APP_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + APP_FOLDERS_TABLE +
            " WHERE " + COL_ID + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, title_id, strlen(title_id), NULL);
            sqlite3_step(res);
        }
        sqlite3_finalize(res);

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    bool UpdatePspPluginSettings(sqlite3 *database, char* rom_path, const PluginSetting *settings)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        int updated = 0;
        std::string sql = std::string("UPDATE ") + PSP_PLUGINS_SETTINGS_TABLE + " SET " + 
                COL_PLUGIN_ENABLE + "=? WHERE " + COL_ROM_PATH + "=? AND " + COL_PLUGIN_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int(res, 1, settings->enable);
            sqlite3_bind_text(res, 2, rom_path, strlen(rom_path), NULL);
            sqlite3_bind_text(res, 3, settings->plugin, strlen(settings->plugin), NULL);
            int step = sqlite3_step(res);
            updated = sqlite3_changes(db);
            sqlite3_finalize(res);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }

        return updated;
    }

    void SavePspPluginSettings(sqlite3 *database, char* rom_path, const PluginSetting *settings)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);
        }

        if (!UpdatePspPluginSettings(db, rom_path, settings))
        {
            sqlite3_stmt *res;
            std::string sql = std::string("INSERT INTO ") + PSP_PLUGINS_SETTINGS_TABLE + "(" + COL_ROM_PATH + "," + 
                COL_PLUGIN_PATH + ", " + COL_PLUGIN_ENABLE + ") " + 
                "VALUES (?, ?, ?)";
            int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);
        
            if (rc == SQLITE_OK)
            {
                sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
                sqlite3_bind_text(res, 2, settings->plugin, strlen(settings->plugin), NULL);
                sqlite3_bind_int(res, 3, settings->enable);
                int step = sqlite3_step(res);
                sqlite3_finalize(res);
            }
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }

    void SavePspPluginSettings(char* rom_path, std::vector<PluginSetting> &plugins)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        for (std::vector<PluginSetting>::iterator it=plugins.begin(); it != plugins.end(); ++it)
        {
            SavePspPluginSettings(db, rom_path, &(*it));
        }

        PerGameUpdated(db);
        sqlite3_close(db);
    }

    void DeletePspPluginSettings(char* rom_path)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + PSP_PLUGINS_SETTINGS_TABLE + " WHERE " + COL_ROM_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        sqlite3_close(db);
    }

    void DeletePspPluginSettings(char* rom_path, PluginSetting *setting)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("DELETE FROM ") + PSP_PLUGINS_SETTINGS_TABLE + " WHERE " + 
            COL_ROM_PATH + "=? AND " + COL_PLUGIN_PATH + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            sqlite3_bind_text(res, 1, setting->plugin, strlen(setting->plugin), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        sqlite3_close(db);
    }

    void GetPspPluginSettings(char* rom_path, std::vector<PluginSetting> &plugins)
    {
        sqlite3 *db;
        sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_PLUGIN_PATH + "," + COL_PLUGIN_ENABLE +
            " FROM " + PSP_PLUGINS_SETTINGS_TABLE + " WHERE " + COL_ROM_PATH + "=?";

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK) {
            sqlite3_bind_text(res, 1, rom_path, strlen(rom_path), NULL);
            while (sqlite3_step(res) == SQLITE_ROW)
            {
                PluginSetting settings;
                sprintf(settings.plugin, "%s", sqlite3_column_text(res, 0));
                settings.enable = sqlite3_column_int(res, 1);
                plugins.push_back(settings);
            }
            sqlite3_finalize(res);
        }

        sqlite3_close(db);
    }

    void PerGameUpdated(sqlite3 *database)
    {
        sqlite3 *db = database;
        if (database == nullptr)
        {
            sqlite3_open(PER_GAME_SETTINGS_DB_FILE, &db);
        }

        sqlite3_stmt *res;
        std::string sql = std::string("SELECT ") + COL_VALUE + " FROM " + 
            UPDATED_TABLE + " WHERE " + COL_UPDATED + "=?";
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        int updated = 0;
        int new_updated = 0;
        if (rc == SQLITE_OK)
        {
            sqlite3_bind_text(res, 1, COL_UPDATED, strlen(COL_UPDATED), NULL);
            int step = sqlite3_step(res);
            if (step == SQLITE_ROW)
            {
                updated = sqlite3_column_int(res, 0);
            }
            sqlite3_finalize(res);
        }
        new_updated = updated + 1;
        if (new_updated > 5000) new_updated = 0;
        sql = std::string("UPDATE ") + UPDATED_TABLE + " SET " + COL_VALUE + "=? WHERE " + COL_UPDATED + "=?";
        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &res, nullptr);

        if (rc == SQLITE_OK)
        {
            sqlite3_bind_int(res, 1, new_updated);
            sqlite3_bind_text(res, 2, COL_UPDATED, strlen(COL_UPDATED), NULL);
            int step = sqlite3_step(res);
            sqlite3_finalize(res);
        }

        if (updated >= 5000)
        {
            rc = sqlite3_exec(db, "VACUUM", NULL, NULL, NULL);
        }

        if (database == nullptr)
        {
            sqlite3_close(db);
        }
    }
}
