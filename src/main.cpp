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
#include "textures.h"
#include "game.h"
#include "config.h"
#include "style.h"
#include "fs.h"
#include "net.h"
#include "ftpclient.h"
#include "debugnet.h"
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
			ImGui_ImplVita2D_DisableButtons(SCE_CTRL_SQUARE | SCE_CTRL_CIRCLE);
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
		// Allow writing to ux0:app/VITASHELL
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

int main(int, char **)
{
	debugNetInit(ip_server,port_server,DEBUG);

	Net::Init();
	Services::Init();
	Services::InitImGui();

	FtpClient *client = new FtpClient();
	int res = client->Connect("192.168.100.14", 21);
	if (res > 0)
	{
		debugNetPrintf(DEBUG,"Success connect to 192.168.100.14\n");
	}

	res = client->Login("cyee", "kramer");
	if (res > 0)
	{
		debugNetPrintf(DEBUG,"Success Login to 192.168.100.14\n");
	}
	client->Quit();

	if (enable_backgrou_music)
	{
		srand(time(NULL));
		int index = rand() % bg_music_list.size();
		Audio_Init(bg_music_list[index].c_str());
	}

	GAME::Init();
	GAME::StartScanGamesThread();

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
