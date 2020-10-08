#ifndef LAUNCHER_GAME_H
#define LAUNCHER_GAME_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "textures.h"

typedef struct {
    char id[10];
    char title[128];
    char category[10];
    char rom_path[192];
    bool favorite = false;
    char type;
    Tex tex;
} Game;

typedef struct {
    char id;
    char title[16];
    std::vector<Game> games;
    std::vector<std::string> valid_title_ids;
    int max_page;
    int page_num;
    int view_mode;
    bool opened;
} GameCategory;

#define FAVORITES 0
#define VITA_GAMES 1
#define PSP_GAMES 2
#define PS1_GAMES 3
#define PS_MIMI_GAMES 4
#define PS_MOBILE_GAMES 5
#define NES_GAMES 6
#define SNES_GAMES 7
#define GB_GAMES 8
#define GBA_GAMES 9
#define N64_GAMES 10
#define PORT_GAMES 11
#define ORIGINAL_GAMES 12
#define UTILITIES 13
#define EMULATORS 14
#define HOMEBREWS 15

#define TOTAL_CATEGORY 16

#define TYPE_BUBBLE 0
#define TYPE_ROM 1

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
    bool Launch(Game *game);
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
