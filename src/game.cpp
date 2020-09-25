#include <cstdint>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <vitasdk.h>

#include "game.h"
#include "sfo.h"
#include "fs.h"
#include "windows.h"
#include "textures.h"

#define GAME_LIST_FILE "ux0:data/SMLA00001/games_list.txt"
#define FAVORITES_FILE "ux0:data/SMLA00001/favorites.txt"

GameCategory game_categories[4];
GameCategory *current_category;

bool game_scan_complete = false;
int games_to_scan = 1;
Game game_scan_inprogress;

namespace GAME {

    void Init() {
    }

    void Scan() {
        if (!FS::FileExists(GAME_LIST_FILE))
        {
            FS::MkDirs("ux0:data/SMLA00001");
            void* fd = FS::Create(GAME_LIST_FILE);

            std::vector<std::string> dirs = FS::ListDir("ux0:app/");
            games_to_scan = dirs.size();

            for(std::size_t i = 0; i < dirs.size(); ++i) {
                char sfo_file[256];
                sprintf(sfo_file, "ux0:app/%s/sce_sys/param.sfo", dirs[i].c_str());
                if (FS::FileExists(sfo_file)) {
                    const auto sfo = FS::Load(sfo_file);

                    Game game;
                    std::string title = std::string(SFO::GetString(sfo.data(), sfo.size(), "TITLE"));
                    std::replace( title.begin(), title.end(), '\n', ' ');

                    sprintf(game.id, "%s", dirs[i].c_str());
                    sprintf(game.title, "%s", title.c_str());
                    sprintf(game.category, "%s", SFO::GetString(sfo.data(), sfo.size(), "CATEGORY"));
                    sprintf(game.icon_path, "ur0:appmeta/%s/icon0.png", dirs[i].c_str());

                    char line[512];
                    sprintf(line, "%s||%s||%s||%s\n", game.id, game.title, game.category, game.icon_path);
                    FS::Write(fd, line, strlen(line));

                    game.tex = no_icon;
                    game_scan_inprogress = game;
                    current_category->games.push_back(game);

                }
            }
            FS::Close(fd);

        }
        else {
            LoadCache();
        }
        SortGames(current_category);
        SetMaxPage(current_category);
        current_category->page_num = 1;

        LoadFavorites();
        SortGames(&game_categories[FAVORITES]);

        for (int i=0; i < current_category->games.size(); i++)
        {
            Game* game = FindGame(&game_categories[FAVORITES], current_category->games[i].id);
            if (game != nullptr)
            {
                current_category->games[i].favorite = true;
            }
        }
    }

    void SetMaxPage(GameCategory *category)
    {
        category->max_page = (category->games.size() + 18 - 1) / 18;
        if (category->max_page == 0)
        {
            category->max_page = 1;
        }
    }

    void Launch(const char *title_id) {
       	char uri[32];
        sprintf(uri, "psgm:play?titleid=%s", title_id);
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelExitProcess(0);
    };

    std::string nextToken(std::vector<char> &buffer, int &nextTokenPos)
    {
        std::string token = "";
        if (nextTokenPos >= buffer.size())
        {
            return NULL;
        }

        for (int i = nextTokenPos; i < buffer.size(); i++)
        {
            if (buffer[i] == '|')
            {
                if ((i+1 < buffer.size()) && (buffer[i+1] == '|'))
                {
                    nextTokenPos = i+2;
                    return token;
                }
                else {
                    token += buffer[i];
                }
            }
            else if (buffer[i] == '\n') {
                nextTokenPos = i+1;
                return token;
            }
            else {
                token += buffer[i];
            }
        }

        return token;
    }

    void LoadCache() {
        std::vector<char> game_buffer = FS::Load(GAME_LIST_FILE);
        int position = 0;
        while (position < game_buffer.size())
        {
            Game game;
            sprintf(game.id, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.title, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.category, "%s", nextToken(game_buffer, position).c_str());
            sprintf(game.icon_path, "%s",  nextToken(game_buffer, position).c_str());
            game.tex = no_icon;
            current_category->games.push_back(game);
            games_to_scan = current_category->games.size();
        }
    };

    void LoadGameImages(int prev_page, int page) {
        int del_page = 0;

        if ((page > prev_page) or (prev_page == current_category->max_page && page == 1))
        {
            del_page = DecrementPage(page, 10);
        } else if ((page < prev_page) or (prev_page == 1 && page == current_category->max_page))
        {
            del_page = IncrementPage(page, 10);
        }

        int high = del_page * 18;
        int low = high - 18;
        if (del_page > 0)
        {
            for (int i=low; (i<high && i < current_category->games.size()); i++)
            {
                Game *game = &current_category->games[i];
                if (game->tex.id != no_icon.id)
                {
                    Textures::Free(&game->tex);
                    game->tex = no_icon;
                }
            }
        }

        high = page * 18;
        low = high - 18;
        for(std::size_t i = low; (i < high && i < current_category->games.size()); i++) {
            Game *game = &current_category->games[i];
            if (game->tex.id == no_icon.id && page == current_category->page_num)
            {
                LoadGameImage(game);
            }
        }
    }

    int IncrementPage(int page, int num_of_pages)
    {
        int new_page = page + num_of_pages;
        if (new_page > current_category->max_page)
        {
            new_page = new_page % current_category->max_page;
        }
        return new_page;
    }

    int DecrementPage(int page, int num_of_pages)
    {
        int new_page = page - num_of_pages;
        if (new_page > 0)
        {
            return new_page;
        }
        return new_page + current_category->max_page;
    }

    void LoadGameImage(Game *game) {
        Tex tex;
        if (FS::FileExists(game->icon_path)) {
            if (Textures::LoadImageFile(game->icon_path, &tex))
            {
                game->tex = tex;
            }
            else
            {
                game->tex = no_icon;
            }
            
            
        } else {
            game->tex = no_icon;
        }
    }

    void Exit() {
    }

	void StartLoadImagesThread(int prev_page_num, int page)
	{
		LoadImagesParams page_param;
		page_param.prev_page_num = prev_page_num;
		page_param.page_num = page;
		load_images_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::LoadImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (load_images_thid >= 0)
			sceKernelStartThread(load_images_thid, sizeof(LoadImagesParams), &page_param);
	}

    int LoadImagesThread(SceSize args, LoadImagesParams *params) {
        sceKernelDelayThread(300000);
        GAME::LoadGameImages(params->prev_page_num, params->page_num);
        return sceKernelExitDeleteThread(0);
    }

    void StartScanGamesThread()
    {
        scan_games_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::ScanGamesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (scan_games_thid >= 0)
			sceKernelStartThread(scan_games_thid, 0, NULL);
    }

    int ScanGamesThread(SceSize args, void *argp)
    {
        game_scan_complete = false;
        GAME::Scan();
        game_scan_complete = true;

        if (game_categories[FAVORITES].games.size() > 0)
        {
            current_category = &game_categories[FAVORITES];
        }
        current_category->page_num = 1;
        GAME::StartLoadImagesThread(1, 1);
        return sceKernelExitDeleteThread(0);
    }

    int GameComparator(const void *v1, const void *v2)
    {
        const Game *p1 = (Game *)v1;
        const Game *p2 = (Game *)v2;
        int p1_len = strlen(p1->title);
        int p2_len = strlen(p2->title);
        int len = p1_len;
        if (p2_len < p1_len)
            len = p2_len;
        return strncmp(p1->title, p2->title, len);
    }

    void DeleteGamesImages(GameCategory *category)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            Game *game = &category->games[i];
            if (game->tex.id != no_icon.id)
            {
                Textures::Free(&game->tex);
                game->tex = no_icon;
            }
        }
    }

    void SaveFavorites()
    {
        void* fd = FS::Create(FAVORITES_FILE);
        for (int i=0; i < game_categories[FAVORITES].games.size(); i++)
        {
            Game* game = &game_categories[FAVORITES].games[i];
            char line[512];
            sprintf(line, "%s||%s||%s||%s\n", game->id, game->title, game->category, game->icon_path);
            FS::Write(fd, line, strlen(line));
        }
        FS::Close(fd);
    }

    void LoadFavorites()
    {
        if (FS::FileExists(FAVORITES_FILE))
        {
            std::vector<char> game_buffer = FS::Load(FAVORITES_FILE);
            int position = 0;
            while (position < game_buffer.size())
            {
                Game game;
                sprintf(game.id, "%s", nextToken(game_buffer, position).c_str());
                sprintf(game.title, "%s", nextToken(game_buffer, position).c_str());
                sprintf(game.category, "%s", nextToken(game_buffer, position).c_str());
                sprintf(game.icon_path, "%s",  nextToken(game_buffer, position).c_str());
                game.tex = no_icon;
                game_categories[FAVORITES].games.push_back(game);
            }
        }
    }

    Game* FindGame(GameCategory *category, char* title_id)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            if (strcmp(title_id, category->games[i].id) == 0)
            {
                return &category->games[i];
            }
        }
        return nullptr;
    }

    int RemoveGameFromCategory(GameCategory *category, char* title_id)
    {
        for (int i=0; i < category->games.size(); i++)
        {
            if (strcmp(title_id, category->games[i].id) == 0)
            {
                category->games.erase(category->games.begin()+i);
                return i;
            }
        }
        return -1;
    }

    void SortGames(GameCategory *category)
    {
        qsort(&category->games[0], category->games.size(), sizeof(Game), GameComparator);
    }
}
