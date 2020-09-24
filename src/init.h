/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __INIT_H__
#define __INIT_H__

typedef struct {
  const char *path;
  void *buffer;
  int size;
  int replace;
} DefaultFile;

extern int is_safe_mode;

extern SceUID kernel_modid, user_modid;

void initLauncher();
int WriteFile(const char *file, const void *buf, int size);

#endif
