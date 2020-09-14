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

Game *games;
int game_count = 0;
Game *selected_game;
int page_num = 1;
int max_page;

namespace GAME {

    void Init() {
        games = reinterpret_cast<Game*>(malloc(MAX_GAMES * sizeof(Game)));
    }

    void Scan() {
        if (!FS::FileExists(GAME_LIST_FILE))
        {
            FS::MkDirs("ux0:data/SMLA00001");
            void* fd = FS::Create(GAME_LIST_FILE);

            std::vector<std::string> dirs = FS::ListDir("ux0:app/");
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

                    games[game_count] = game;
                    games[game_count].tex = no_icon;
                    game_count++;

                }
            }
            FS::Close(fd);

        }
        else {
            LoadCache();
        }
        qsort(games, game_count, sizeof(Game), GameComparator);
        max_page = (game_count + 18 - 1) / 18;
        selected_game = &games[0];
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
            sprintf(games[game_count].id, "%s", nextToken(game_buffer, position).c_str());
            sprintf(games[game_count].title, "%s", nextToken(game_buffer, position).c_str());
            sprintf(games[game_count].category, "%s", nextToken(game_buffer, position).c_str());
            sprintf(games[game_count].icon_path, "%s",  nextToken(game_buffer, position).c_str());
            games[game_count].tex = no_icon;
            game_count++;
        }
    };

    void LoadGameImages(int page) {
        int high = page * 18;
        int low = high - 18;
        for(std::size_t i = low; (i < high && i < game_count); i++) {
            Game *game = &games[i];
            if (game->tex.id == no_icon.id && page == page_num)
            {
                LoadGameImage(game);
            }
        }
    }

    void LoadGameImage(Game *game) {
        Tex tex;
        if (FS::FileExists(game->icon_path)) {
            Textures::LoadImageFile(game->icon_path, &tex);
            game->tex = tex;
        } else {
            game->tex = no_icon;
        }
    }

    void Exit() {
        free(games);
    }

    int LoadImagesThread(SceSize args, LoadImagesParams *params) {
        sceKernelDelayThread(500*1000);
        GAME::LoadGameImages(params->page_num);
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
}
