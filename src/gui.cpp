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
	void StartLoadImagesThread(int prev_page_num, int page)
	{
		LoadImagesParams page_param;
		page_param.prev_page_num = prev_page_num;
		page_param.page_num = page;
		load_images_thid = sceKernelCreateThread("load_images_thread", (SceKernelThreadEntry)GAME::LoadImagesThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (load_images_thid >= 0)
			sceKernelStartThread(load_images_thid, sizeof(LoadImagesParams), &page_param);
	}

	int RenderLoop(void) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.NavInputs[ImGuiNavInput_Menu] = 0.0f;
		
		bool done = false, focus = false, first_item = true;

		//GAME::LoadGameImages(page_num);
		StartLoadImagesThread(page_num, page_num);
		SceCtrlData pad_prev;
		while (!done) {
			vita2d_start_drawing();
			vita2d_clear_screen();

			SceCtrlData pad;
			sceCtrlPeekBufferNegative(0, &pad, 1);
			if ((pad_prev.buttons & SCE_CTRL_LTRIGGER) && !(pad.buttons & SCE_CTRL_LTRIGGER))
			{
				int prev_page = page_num;
				page_num = GAME::DecrementPage(page_num, 1);
				//GAME::LoadGameImages(page_num);
				StartLoadImagesThread(prev_page, page_num);
			} else if ((pad_prev.buttons & SCE_CTRL_RTRIGGER) && !(pad.buttons & SCE_CTRL_RTRIGGER))
			{
				int prev_page = page_num;
				page_num = GAME::IncrementPage(page_num, 1);
				//GAME::LoadGameImages(page_num);
				StartLoadImagesThread(prev_page, page_num);
			}
			pad_prev = pad;

			ImGui_ImplVita2D_NewFrame();
			
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
