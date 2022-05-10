#include <imgui_vita.h>
#include <string>
#include <vector>
#include <stdio.h>

#include "style.h"
#include "config.h"
#include "fs.h"

extern "C" {
	#include "inifile.h"
}

char style_path[128];
char style_name[64];

namespace Style {
    void LoadStyle(const char *style_path)
    {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        OpenIniFile(style_path);

        colors[ImGuiCol_Text]                   = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Text, DEFAULT_COLOR));
        colors[ImGuiCol_TextDisabled]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TextDisabled, DEFAULT_COLOR));
        colors[ImGuiCol_WindowBg]               = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_WindowBg, DEFAULT_COLOR));
        colors[ImGuiCol_ChildBg]                = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ChildBg, DEFAULT_COLOR));
        colors[ImGuiCol_PopupBg]                = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_PopupBg, DEFAULT_COLOR));
        colors[ImGuiCol_Border]                 = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Border, DEFAULT_COLOR));
        colors[ImGuiCol_BorderShadow]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_BorderShadow, DEFAULT_COLOR));
        colors[ImGuiCol_FrameBg]                = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_FrameBg, DEFAULT_COLOR));
        colors[ImGuiCol_FrameBgHovered]         = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_FrameBgHovered, DEFAULT_COLOR));
        colors[ImGuiCol_FrameBgActive]          = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_FrameBgActive, DEFAULT_COLOR));
        colors[ImGuiCol_TitleBg]                = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TitleBg, DEFAULT_COLOR));
        colors[ImGuiCol_TitleBgActive]          = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TitleBgActive, DEFAULT_COLOR));
        colors[ImGuiCol_TitleBgCollapsed]       = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TitleBgCollapsed, DEFAULT_COLOR));
        colors[ImGuiCol_MenuBarBg]              = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_MenuBarBg, DEFAULT_COLOR));
        colors[ImGuiCol_ScrollbarBg]            = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ScrollbarBg, DEFAULT_COLOR));
        colors[ImGuiCol_ScrollbarGrab]          = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ScrollbarGrab, DEFAULT_COLOR));
        colors[ImGuiCol_ScrollbarGrabHovered]   = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ScrollbarGrabHovered, DEFAULT_COLOR));
        colors[ImGuiCol_ScrollbarGrabActive]    = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ScrollbarGrabActive, DEFAULT_COLOR));
        colors[ImGuiCol_CheckMark]              = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_CheckMark, DEFAULT_COLOR));
        colors[ImGuiCol_SliderGrab]             = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_SliderGrab, DEFAULT_COLOR));
        colors[ImGuiCol_SliderGrabActive]       = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_SliderGrabActive, DEFAULT_COLOR));
        colors[ImGuiCol_Button]                 = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Button, DEFAULT_COLOR));
        colors[ImGuiCol_ButtonHovered]          = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ButtonHovered, DEFAULT_COLOR));
        colors[ImGuiCol_ButtonActive]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ButtonActive, DEFAULT_COLOR));
        colors[ImGuiCol_Header]                 = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Header, DEFAULT_COLOR));
        colors[ImGuiCol_HeaderHovered]          = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_HeaderHovered, DEFAULT_COLOR));
        colors[ImGuiCol_HeaderActive]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_HeaderActive, DEFAULT_COLOR));
        colors[ImGuiCol_Separator]              = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Separator, DEFAULT_COLOR));
        colors[ImGuiCol_SeparatorHovered]       = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_SeparatorHovered, DEFAULT_COLOR));
        colors[ImGuiCol_SeparatorActive]        = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_SeparatorActive, DEFAULT_COLOR));
        colors[ImGuiCol_ResizeGrip]             = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ResizeGrip, DEFAULT_COLOR));
        colors[ImGuiCol_ResizeGripHovered]      = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ResizeGripHovered, DEFAULT_COLOR));
        colors[ImGuiCol_ResizeGripActive]       = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ResizeGripActive, DEFAULT_COLOR));
        colors[ImGuiCol_Tab]                    = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_Tab, DEFAULT_COLOR));
        colors[ImGuiCol_TabHovered]             = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TabHovered, DEFAULT_COLOR));
        colors[ImGuiCol_TabActive]              = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TabActive, DEFAULT_COLOR));
        colors[ImGuiCol_TabUnfocused]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TabUnfocused, DEFAULT_COLOR));
        colors[ImGuiCol_TabUnfocusedActive]     = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TabUnfocusedActive, DEFAULT_COLOR));
        colors[ImGuiCol_TextSelectedBg]         = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_TextSelectedBg, DEFAULT_COLOR));
        colors[ImGuiCol_NavHighlight]           = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_NavHighlight, DEFAULT_COLOR));
        colors[ImGuiCol_NavWindowingHighlight]  = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_NavWindowingHighlight, DEFAULT_COLOR));
        colors[ImGuiCol_NavWindowingDimBg]      = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_NavWindowingDimBg, DEFAULT_COLOR));
        colors[ImGuiCol_ModalWindowDimBg]       = GetColor(ReadString(CONFIG_STYLE, CONFIG_STYLE_ModalWindowDimBg, DEFAULT_COLOR));

        CloseIniFile();
    }

    ImVec4 GetColor(const char *color)
    {
        std::vector<float> rgba;

        std::string color_code = "";
        int length = strlen(color);
        for (int i=0; i<length; i++)
        {
            char c = color[i];
            if (c != ' ' && c != '\t' && c != ',')
            {
                color_code += c;
            }
            
            if (c == ',' || i==length-1)
            {
                rgba.push_back(atof(color_code.c_str()));
                color_code = "";
            }
        }

        return ImVec4(rgba[0],rgba[1],rgba[2],rgba[3]);
    }

    void SetStylePath(const char* style_name_p)
    {
        
        if (strcmp(style_name_p, CONFIG_DEFAULT_STYLE_NAME) == 0)
        {
            sprintf(style_path, "%s", DEFAULT_STYLE_PATH);
        }
        else
        {
            sprintf(style_path, "%s/%s.ini", STYLES_FOLDER, style_name_p);
            if (!FS::FileExists(style_path))
            {
                sprintf(style_path, "%s", DEFAULT_STYLE_PATH);
                sprintf(style_name, "%s", CONFIG_DEFAULT_STYLE_NAME);
            }
        }
    }
}