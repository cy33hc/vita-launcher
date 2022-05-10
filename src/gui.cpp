#include <imgui_vita.h>
#include <stdio.h>
#include <vita2d.h>
#include <vitaGL.h>
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
			if (gui_mode < GUI_MODE_IME)
			{
				ImGui_ImplVitaGL_NewFrame();
			
				if (gui_mode == GUI_MODE_SCAN)
				{
					Windows::GameScanWindow();
				}
				else if (gui_mode == GUI_MODE_LAUNCHER)
				{
					Windows::HandleLauncherWindowInput();
					Windows::LauncherWindow();
				}
				glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
				ImGui::Render();
				ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
				vglSwapBuffers(GL_FALSE);
			}
			else
			{
				Windows::HandleImeInput();
			}
		}
		
		return 0;
	}
}
