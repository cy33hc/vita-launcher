#include <imgui_vita2d/imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>
#include "game.h"
#include "windows.h"
#include "gui.h"
#include "textures.h"

// Global var used across windows/popups
//MenuItem item;


bool done = false;
int gui_mode = GUI_MODE_SCAN;

namespace GUI {
	int RenderLoop(void) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		
		bool done = false;
		view_mode = current_category->view_mode;
		
		Windows::Init();
		while (!done) {
			vita2d_start_drawing();
			vita2d_clear_screen();

			if (gui_mode < GUI_MODE_IME)
			{
				ImGui_ImplVita2D_NewFrame();
			}
			
			if (gui_mode == GUI_MODE_SCAN)
			{
				Windows::GameScanWindow();
			}
			else if (gui_mode == GUI_MODE_LAUNCHER)
			{
				Windows::HandleLauncherWindowInput();
				Windows::LauncherWindow();
			} else if (gui_mode == GUI_MODE_IME)
			{
				Windows::HandleImeInput();
			}
			
			if (gui_mode < GUI_MODE_IME)
			{
				ImGui::Render();
				ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());
			}

			vita2d_end_drawing();
			vita2d_common_dialog_update();
			vita2d_swap_buffers();
			sceDisplayWaitVblankStart();
		}
		
		return 0;
	}
}
