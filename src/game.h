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
    char category[10];
    char icon_path[96];
    bool favorite = false;
    Tex tex;
} Game;

typedef struct {
    char id;
    char title[16];
    std::vector<Game> games;
    int max_page;
    int page_num;
    int view_mode;
} GameCategory;

#define VITA_GAMES 0
#define PSP_GAMES 1
#define HOMEBREWS 2
#define FAVORITES 3

extern GameCategory game_categories[];
extern GameCategory *current_category;
extern bool game_scan_complete;
extern int games_to_scan;
extern Game game_scan_inprogress;
extern std::string psp;
extern std::string vita;
extern std::string homebrew;

static SceUID load_images_thid = -1;
static SceUID scan_games_thid = -1;

typedef struct LoadImagesParams {
  int category;
  int prev_page_num;
  int page_num;
} LoadImagesParams;

namespace GAME {
    int GameComparator(const void *v1, const void *v2);
    void Init();
    void Scan();
    bool GetGameDetails(const char *id, Game *game);
    bool Launch(const char *id);
    void LoadGamesCache();
    void SaveGamesCache();
    void LoadGameImages(int category, int prev_page, int page_num);
    void LoadGameImage(Game *game);
    void Exit();
    int LoadImagesThread(SceSize args, LoadImagesParams *argp);
    int IncrementPage(int page, int num_of_pages);
    int DecrementPage(int page, int num_of_pages);
    void StartLoadImagesThread(int category, int prev_page_num, int page);
    int ScanGamesThread(SceSize args, void *argp);
    void StartScanGamesThread();
    void DeleteGamesImages(GameCategory *category);
    void SetMaxPage(GameCategory *category);
    void SaveFavorites();
    void LoadFavorites();
    Game* FindGame(GameCategory *category, char* title_id);
    int FindGamePosition(GameCategory *category, char* title_id);
    int RemoveGameFromCategory(GameCategory *category, char* title_id);
    void SortGames(GameCategory *category);
    void RefreshGames();
    const char* GetGameCategory(const char *id);
}

#endif
