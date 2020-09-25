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

namespace Services {
	int InitImGui(void) {

		// Setup ImGui binding
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.MouseDrawCursor = false;
		ImGui::StyleColorsDark();
		ImGui_ImplVita2D_Init();

		ImGui_ImplVita2D_TouchUsage(true);
		ImGui_ImplVita2D_UseIndirectFrontTouch(false);
		ImGui_ImplVita2D_UseRearTouch(false);
		ImGui_ImplVita2D_GamepadUsage(true);
		ImGui_ImplVita2D_MouseStickUsage(false);
		ImGui_ImplVita2D_DisableButtons(SCE_CTRL_TRIANGLE | SCE_CTRL_SQUARE);
		
		Textures::Init();

		return 0;
	}
	
	void ExitImGui(void) {
		Textures::Exit();

		// Cleanup
		ImGui_ImplVita2D_Shutdown();
		ImGui::DestroyContext();
		
	}
	
	int Init(void) {
		vita2d_init();
		vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));

		game_categories[VITA_GAMES].id = VITA_GAMES;
		sprintf(game_categories[VITA_GAMES].title, "%s", "Vita Games");
		game_categories[VITA_GAMES].page_num = 1;
		game_categories[VITA_GAMES].max_page = 0;

		game_categories[PSP_GAMES].id = PSP_GAMES;
		sprintf(game_categories[PSP_GAMES].title, "%s", "PSP Games");
		game_categories[PSP_GAMES].page_num = 1;
		game_categories[PSP_GAMES].max_page = 0;

		game_categories[HOMEBREWS].id = HOMEBREWS;
		sprintf(game_categories[HOMEBREWS].title, "%s", "Homebrews");
		game_categories[HOMEBREWS].page_num = 1;
		game_categories[HOMEBREWS].max_page = 0;

		game_categories[FAVORITES].id = FAVORITES;
		sprintf(game_categories[FAVORITES].title, "%s", "Favorites");
		game_categories[FAVORITES].page_num = 1;
		game_categories[FAVORITES].max_page = 0;

		current_category = &game_categories[VITA_GAMES];
		return 0;
	}
	
	void Exit(void) {
		vita2d_fini();
	}
}

#define ip_server "192.168.100.14"
#define port_server 18194

int main(int, char**)
{
	Services::Init();
	Services::InitImGui();

	GAME::Init();
	GAME::StartScanGamesThread();

	GUI::RenderLoop();

	GAME::Exit();
	Services::ExitImGui();
	Services::Exit();
	return 0;
}
