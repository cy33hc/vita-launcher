#include <imgui_vita.h>
#include <vita2d.h>
#include <vitaGL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "textures.h"
#include "config.h"
#include "gui.h"

Tex no_icon;
Tex favorite_icon;
Tex square_icon;
Tex triangle_icon;
Tex circle_icon;
Tex cross_icon;
Tex start_icon;
Tex folder_icon;
Tex selected_icon;
Tex redbar_icon;
Tex greenbar_icon;

namespace Textures {
	
	bool LoadImageFile(const std::string filename, Tex *texture)
	{
		// Load from file
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename.c_str(), &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		texture->width = image_width;
		texture->height = image_height;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		stbi_image_free(image_data);


		return true;
	}
	
	void Init(void) {
		Textures::LoadImageFile("ux0:app/SMLA00001/noicon.png", &no_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/favorite.png", &favorite_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/square.png", &square_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/triangle.png", &triangle_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/circle.png", &circle_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/cross.png", &cross_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/start.png", &start_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/folder.png", &folder_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/selected.png", &selected_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/redbar.png", &redbar_icon);
		Textures::LoadImageFile("ux0:app/SMLA00001/greenbar.png", &greenbar_icon);
	}

	void Exit(void) {
		Free(&no_icon);
		Free(&favorite_icon);
		Free(&square_icon);
		Free(&triangle_icon);
		Free(&circle_icon);
		Free(&cross_icon);
		Free(&start_icon);
		Free(&folder_icon);
		Free(&selected_icon);
		Free(&redbar_icon);
		Free(&greenbar_icon);
	}

	void Free(Tex *texture) {
		if (texture->id != no_icon.id && texture->id != 0)
		{
			glDeleteTextures(1, &texture->id);
			glFlush();
		}
	}
	
}
