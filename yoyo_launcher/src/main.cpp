#include <vitasdk.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "debugnet.h"

int _newlib_heap_size_user = 256 * 1024 * 1024;

#define GMS_GAMES_PATH "ux0:data/gms"
#define YOYO_LAUNCH_FILE_PATH "ux0:data/gms/launch.txt"

struct GameSettings {
	char name[128];
	float size;
	bool bilinear;
	bool gles1;
	bool skip_splash;
	bool compress_textures;
	bool fake_win_mode;
	bool debug_mode;
	bool debug_shaders;
	bool mem_extended;
	bool newlib_extended;
	bool video_support;
};

void LoadYoYoSettings(char *game, GameSettings *settings)
{
	char configFile[512];
	char buffer[30];
	int value;
	
	sprintf(configFile, "%s/%s/yyl.cfg", GMS_GAMES_PATH, game);
	FILE *config = fopen(configFile, "r");

	settings->bilinear = false;
	settings->compress_textures = false;
	settings->debug_mode = false;
	settings->debug_shaders = false;
	settings->fake_win_mode = false;
	settings->gles1 = false;
	settings->mem_extended = false;
	settings->newlib_extended = false;
	settings->skip_splash = false;
	settings->video_support = false;
	if (config) {
		while (EOF != fscanf(config, "%[^=]=%d\n", buffer, &value)) {
			if (strcmp("forceGLES1", buffer) == 0) settings->gles1 = (bool)value;
			else if (strcmp("forceBilinear", buffer) == 0) settings->bilinear = (bool)value;
			else if (strcmp("winMode", buffer) == 0) settings->fake_win_mode = (bool)value;
			else if (strcmp("debugShaders", buffer) == 0) settings->debug_shaders = (bool)value;
			else if (strcmp("compressTextures", buffer) == 0) settings->compress_textures = (bool)value;
			else if (strcmp("debugMode", buffer) == 0) settings->debug_mode = (bool)value;
			else if (strcmp("noSplash", buffer) == 0) settings->skip_splash = (bool)value;
			else if (strcmp("maximizeMem", buffer) == 0) settings->mem_extended = (bool)value;
			else if (strcmp("maximizeNewlib", buffer) == 0) settings->newlib_extended = (bool)value;
			else if (strcmp("videoSupport", buffer) == 0) settings->video_support = (bool)value;
		}
		fclose(config);
	}
}

void PrepareForBoot(char *game, GameSettings *settings)
{
	char configFile[512];
	char buffer[128];

	if (settings->newlib_extended) {
		FS::Save("ux0:data/gms/newlib.cfg", "1", 1);
	}

	FS::Save(YOYO_LAUNCH_FILE_PATH, game, strlen(game));
}

int main(int, char **)
{
	char boot_params[1024];
	char launch_item[512];
	GameSettings settings;

	sceAppMgrGetAppParam(boot_params);
	if (strstr(boot_params,"psgm:play") && strstr(boot_params, "&param=")) {
		strncpy(launch_item, strstr(boot_params, "&param=") + 7, 512);
	}
	else
	{
		return 0;
	}

	LoadYoYoSettings(launch_item, &settings);
	PrepareForBoot(launch_item, &settings);

	sceAppMgrLoadExec(settings.video_support ? "app0:/loader2.bin" : "app0:/loader.bin", NULL, NULL);
	return 0;
}
