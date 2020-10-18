

#ifndef _ISO_H_
#define _ISO_H_
#include <imgui_vita2d/imgui_vita.h>
#include <string>
#include <vector>
extern "C" {
	#include "inifile.h"
}

#define DEFAULT_STYLE_PATH                  "ux0:app/SMLA00001/default_style.ini"
#define STYLES_FOLDER                       "ux0:data/SMLA00001/styles"

#define CONFIG_STYLE                        "Style"
#define DEFAULT_COLOR                       "1.00,1.00,1.00,1.00"

#define CONFIG_STYLE_Text                   "Text"
#define CONFIG_STYLE_TextDisabled           "TextDisabled"
#define CONFIG_STYLE_WindowBg               "WindowBackgroud"
#define CONFIG_STYLE_ChildBg                "ChildBackground"
#define CONFIG_STYLE_PopupBg                "PopupBackground"
#define CONFIG_STYLE_Border                 "Border"
#define CONFIG_STYLE_BorderShadow           "BorderShadhow"
#define CONFIG_STYLE_FrameBg                "FrameBackground"
#define CONFIG_STYLE_FrameBgHovered         "FrameBackgroundHovered"
#define CONFIG_STYLE_FrameBgActive          "FrameBackgroundActive"
#define CONFIG_STYLE_TitleBg                "TitleBackground"
#define CONFIG_STYLE_TitleBgActive          "TitleBackgroundActive"
#define CONFIG_STYLE_TitleBgCollapsed       "TitleBackgroundCollapsed"
#define CONFIG_STYLE_MenuBarBg              "MenuBarBackground"
#define CONFIG_STYLE_ScrollbarBg            "ScrollbarBackground"
#define CONFIG_STYLE_ScrollbarGrab          "ScrollbarGrab"
#define CONFIG_STYLE_ScrollbarGrabHovered   "ScrollbarGrabHovered"
#define CONFIG_STYLE_ScrollbarGrabActive    "ScrollbarGrabActive"
#define CONFIG_STYLE_CheckMark              "CheckMark"
#define CONFIG_STYLE_SliderGrab             "SliderGrab"
#define CONFIG_STYLE_SliderGrabActive       "SliderGrabActive"
#define CONFIG_STYLE_Button                 "Button"
#define CONFIG_STYLE_ButtonHovered          "ButtonHovered"
#define CONFIG_STYLE_ButtonActive           "ButtonActive"
#define CONFIG_STYLE_Header                 "Header"
#define CONFIG_STYLE_HeaderHovered          "HeaderHovered"
#define CONFIG_STYLE_HeaderActive           "HeaderActive"
#define CONFIG_STYLE_Separator              "Separator"
#define CONFIG_STYLE_SeparatorHovered       "SeparatorHovered"
#define CONFIG_STYLE_SeparatorActive        "SeparatorActive"
#define CONFIG_STYLE_ResizeGrip             "ResizeGrip"
#define CONFIG_STYLE_ResizeGripHovered      "ResizeHovered"
#define CONFIG_STYLE_ResizeGripActive       "ResizeGripActive"
#define CONFIG_STYLE_Tab                    "Tab"
#define CONFIG_STYLE_TabHovered             "TabHovered"
#define CONFIG_STYLE_TabActive              "TabActive"
#define CONFIG_STYLE_TabUnfocused           "TabUnfocused"
#define CONFIG_STYLE_TabUnfocusedActive     "TabUnfocusedActive"
#define CONFIG_STYLE_PlotLines              "PlotLines"
#define CONFIG_STYLE_PlotLinesHovered       "PlotLinesHovered"
#define CONFIG_STYLE_PlotHistogram          "PlotHistogram"
#define CONFIG_STYLE_PlotHistogramHovered   "PlotHistogramHovered"
#define CONFIG_STYLE_TextSelectedBg         "TextSelectedBackground"
#define CONFIG_STYLE_DragDropTarget         "DragDropTarget"
#define CONFIG_STYLE_NavHighlight           "NavigationHighlight"
#define CONFIG_STYLE_NavWindowingHighlight  "NavigationWindowingHighlight"
#define CONFIG_STYLE_NavWindowingDimBg      "NavigationWindowingDimBackground"
#define CONFIG_STYLE_ModalWindowDimBg       "ModalWindowDimBackground"

extern char style_name[];
extern char style_path[];

namespace Style {
    void LoadStyle(const char *style_path);
    void SetStylePath(const char *style_name);
    ImVec4 GetColor(const char *color);
}
#endif