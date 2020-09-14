#ifndef LAUNCHER_GAME_H
#define LAUNCHER_GAME_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "textures.h"

typedef struct {
    char id[10];
    char title[256];
    char category[5];
    char icon_path[96];
    Tex tex;
} Game;

#define MAX_GAMES 2000

extern Game *games;
extern int game_count;
extern Game *selected_game;
extern int page_num;
extern int max_page;

static SceUID load_images_thid = -1;

typedef struct LoadImagesParams {
  int page_num;
} LoadImagesParams;

namespace GAME {
    int GameComparator(const void *v1, const void *v2);
    void Init();
    void Scan();
    void Launch(const char *id);
    void LoadCache();
    void LoadGameImages(int page_num);
    void LoadGameImage(Game *game);
    void Exit();
    int LoadImagesThread(SceSize args, LoadImagesParams *argp);
}

#endif
