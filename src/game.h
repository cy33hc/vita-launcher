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

typedef struct {
    char title[16];
    std::vector<Game> games;
    int max_page;
    int page_num;
} GameCategory;

#define MAX_GAMES 2000
#define VITA_GAMES 0
#define PSP_GAMES 1
#define HOMEBREWS 2
#define FAVORITES 3

extern GameCategory game_categories[];
extern GameCategory *current_games;
extern bool game_scan_complete;
extern int games_to_scan;
extern Game game_scan_inprogress;

static SceUID load_images_thid = -1;
static SceUID scan_games_thid = -1;

typedef struct LoadImagesParams {
  int prev_page_num;
  int page_num;
} LoadImagesParams;

namespace GAME {
    int GameComparator(const void *v1, const void *v2);
    void Init();
    void Scan();
    void Launch(const char *id);
    void LoadCache();
    void LoadGameImages(int prev_page, int page_num);
    void LoadGameImage(Game *game);
    void Exit();
    int LoadImagesThread(SceSize args, LoadImagesParams *argp);
    int IncrementPage(int page, int num_of_pages);
    int DecrementPage(int page, int num_of_pages);
    void StartLoadImagesThread(int prev_page_num, int page);
    int ScanGamesThread(SceSize args, void *argp);
    void StartScanGamesThread();
}

#endif
