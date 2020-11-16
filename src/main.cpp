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
#include "debugnet.h"

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

	GAME::Init();
	GAME::StartScanGamesThread();

	GUI::RenderLoop();

	GAME::Exit();
	Services::ExitImGui();
	Services::Exit();
	Net::Exit();

	return 0;
}
