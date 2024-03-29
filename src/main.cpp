// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the sdl_opengl3_example/ folder**
// See imgui_impl_sdl.cpp for details.

#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>

#include "gui.h"
#include "windows.h"
#include "textures.h"
#include "game.h"
#include "config.h"
#include "style.h"
#include "fs.h"
#include "net.h"
#include "ftpclient.h"
#include "updater.h"
//#include "debugnet.h"

extern "C" {
	#include "audio.h"
}
namespace Services
{
	int InitImGui(void)
	{

		// Setup ImGui binding
		ImGui::CreateContext();

		ImGuiIO &io = ImGui::GetIO();
		(void)io;
		io.MouseDrawCursor = false;
		ImGui::StyleColorsDark();
		auto &style = ImGui::GetStyle();
		style.AntiAliasedLinesUseTex = false;
		style.AntiAliasedLines = true;
		style.AntiAliasedFill = true;

		static const ImWchar ranges[] = { // All languages with chinese included
			0x0020, 0x00FF, // Basic Latin + Latin Supplement
			0x0100, 0x024F, // Latin Extended
			0x0370, 0x03FF, // Greek
			0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
			0x0590, 0x05FF, // Hebrew
			0x1E00, 0x1EFF, // Latin Extended Additional
			0x1F00, 0x1FFF, // Greek Extended
			0x2000, 0x206F, // General Punctuation
			0x2100, 0x214F, // Letterlike Symbols
			0x2460, 0x24FF, // Enclosed Alphanumerics
			0x2DE0, 0x2DFF, // Cyrillic Extended-A
			0x2E80, 0x2EFF, // CJK Radicals Supplement
			0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
			0x31F0, 0x31FF, // Katakana Phonetic Extensions
			0x3400, 0x4DBF, // CJK Rare
			0x4E00, 0x9FFF, // CJK Ideograms
			0xA640, 0xA69F, // Cyrillic Extended-B
			0xF900, 0xFAFF, // CJK Compatibility Ideographs
			0xFF00, 0xFFEF, // Half-width characters
			0,
		};

		ImFontConfig config;
		config.MergeMode = true;
		io.Fonts->AddFontFromFileTTF(
			"sa0:/data/font/pvf/ltn0.pvf",
			16.0f,
			NULL,
			ranges);
		io.Fonts->AddFontFromFileTTF(
			"sa0:/data/font/pvf/jpn0.pvf",
			16.0f,
			&config,
			io.Fonts->GetGlyphRangesJapanese());
		io.Fonts->Build();

		Style::LoadStyle(style_path);

		ImGui_ImplVita2D_Init();

		ImGui_ImplVita2D_TouchUsage(true);
		ImGui_ImplVita2D_UseIndirectFrontTouch(false);
		ImGui_ImplVita2D_UseRearTouch(false);
		ImGui_ImplVita2D_GamepadUsage(true);
		ImGui_ImplVita2D_MouseStickUsage(false);
		ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE);
		if (!show_categories_as_tabs)
		{
			if (swap_xo)
			{
				ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CROSS);
			}
			else
			{
				ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
			}
		}
		ImGui_ImplVita2D_SwapXO(swap_xo);

		Textures::Init();

		return 0;
	}

	void ExitImGui(void)
	{
		Textures::Exit();

		// Cleanup
		ImGui_ImplVita2D_Shutdown();
		ImGui::DestroyContext();
	}

	void initSceAppUtil()
	{
		// Init SceAppUtil
		SceAppUtilInitParam init_param;
		SceAppUtilBootParam boot_param;
		memset(&init_param, 0, sizeof(SceAppUtilInitParam));
		memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
		sceAppUtilInit(&init_param, &boot_param);

		// Set common dialog config
		SceCommonDialogConfigParam config;
		sceCommonDialogConfigParamInit(&config);
		sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
		sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
		sceCommonDialogSetConfigParam(&config);

		uint32_t scepaf_argp[] = { 0x400000, 0xEA60, 0x40000, 0, 0 };

		SceSysmoduleOpt option;
        option.flags = 0;
        option.result = (int *)&option.flags;
		sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(scepaf_argp), scepaf_argp, &option);
		sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
		scePromoterUtilityInit();
	}

	int Init(void)
	{
		scePowerSetArmClockFrequency(444);
		scePowerSetBusClockFrequency(222);
		scePowerSetGpuClockFrequency(222);
		scePowerSetGpuXbarClockFrequency(166);

		// Allow writing to ux0:app
		sceAppMgrUmount("app0:");
		sceAppMgrUmount("savedata0:");

		vita2d_init();
		vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

		initSceAppUtil();

		CONFIG::LoadConfig();

		current_category = &game_categories[VITA_GAMES];

		return 0;
	}

	void Exit(void)
	{
		// Shutdown AppUtil
		sceAppUtilShutdown();
		scePromoterUtilityExit();
		sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
		vita2d_fini();
	}
} // namespace Services

#define ip_server "192.168.100.14"
#define port_server 18194

int _newlib_heap_size_user = 128 * 1024 * 1024;

int main(int, char **)
{
	//debugNetInit(ip_server,port_server, DEBUG);

	Net::Init();
	Services::Init();
	Services::InitImGui();

	if (enable_backgrou_music)
	{
		srand(time(NULL));
		int index = rand() % bg_music_list.size();
		Audio_Init(bg_music_list[index].c_str());
	}

	GAME::Init();
	GAME::StartScanGamesThread();
	GAME::StartGetCacheStateThread();
	GAME::MigratePSPCache();
	Updater::StartInstallerThread();
	
	GUI::RenderLoop();

	GAME::Exit();
	if (enable_backgrou_music)
	{
		Audio_Term();
	}
	Services::ExitImGui();
	Services::Exit();
	Net::Exit();

	return 0;
}
