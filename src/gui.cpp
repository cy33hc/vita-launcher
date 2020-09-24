#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>
#include "game.h"
#include "windows.h"
#include "textures.h"

// Global var used across windows/popups
//MenuItem item;

#define MODE_SCAN 0
#define MODE_LAUNCHER 1

bool done = false;
static int mode = MODE_SCAN;

namespace GUI {
	int RenderLoop(void) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.NavInputs[ImGuiNavInput_Menu] = 0.0f;
		
		bool done = false;

		while (!done) {
			vita2d_start_drawing();
			vita2d_clear_screen();

			ImGui_ImplVita2D_NewFrame();

			if (!game_scan_complete)
			{
				Windows::GameScanWindow();
			}
			else
			{
				Windows::HandleLauncherWindowInput();
				Windows::LauncherWindow();
			}
			
			ImGui::Render();
			ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

			vita2d_end_drawing();
			vita2d_swap_buffers();
			sceDisplayWaitVblankStart();
		}
		
		return 0;
	}
}
