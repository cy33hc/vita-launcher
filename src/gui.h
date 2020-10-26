#ifndef LAUNCHER_GUI_H
#define LAUNCHER_GUI_H

#include <string>
#include "game.h"

#define GUI_MODE_SCAN 0
#define GUI_MODE_LAUNCHER 1
#define GUI_MODE_IME 2

extern int gui_mode;

namespace GUI {
    int RenderLoop(void);
}

#endif
