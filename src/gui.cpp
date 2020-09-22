#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>
#include "game.h"
#include "windows.h"
#include "textures.h"

// Global var used across windows/popups
//MenuItem item;

bool done = false;

namespace GUI {
	int RenderLoop(void) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.NavInputs[ImGuiNavInput_Menu] = 0.0f;
		
		bool done = false, focus = false, first_item = true;

		GAME::StartLoadImagesThread(page_num, page_num);
		while (!done) {
			vita2d_start_drawing();
			vita2d_clear_screen();

			ImGui_ImplVita2D_NewFrame();

			Windows::HandleLauncherWindowInput();
			Windows::LauncherWindow(&focus, &first_item);
			
			ImGui::Render();
			ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

			vita2d_end_drawing();
			vita2d_swap_buffers();
			sceDisplayWaitVblankStart();
		}
		
		return 0;
	}
}
