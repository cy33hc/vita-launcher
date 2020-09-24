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

#include <vitasdk.h>
#include "init.h"
#include <taihen.h>
#include <vitashell_user.h>
#include "vitashell_error.h"
#include "pfs.h"

extern unsigned char _binary_modules_kernel_kernel_skprx_start;
extern unsigned char _binary_modules_kernel_kernel_skprx_size;
extern unsigned char _binary_modules_user_user_suprx_start;
extern unsigned char _binary_modules_user_user_suprx_size;
extern unsigned char _binary_modules_patch_patch_skprx_start;
extern unsigned char _binary_modules_patch_patch_skprx_size;

#define DEFAULT_FILE(path, name, replace) { path, (void *)&_binary_resources_##name##_start, (int)&_binary_resources_##name##_size, replace }

static DefaultFile default_files[] = {
  { "ux0:data/SMLA00001/module/kernel.skprx",    (void *)&_binary_modules_kernel_kernel_skprx_start,
                                               (int)&_binary_modules_kernel_kernel_skprx_size, 1 },
  { "ux0:data/SMLA00001/module/user.suprx",      (void *)&_binary_modules_user_user_suprx_start,
                                               (int)&_binary_modules_user_user_suprx_size, 1 },
  { "ux0:data/SMLA00001/module/patch.skprx",     (void *)&_binary_modules_patch_patch_skprx_start,
                                               (int)&_binary_modules_patch_patch_skprx_size, 1 },
};

SceUID patch_modid = -1, kernel_modid = -1, user_modid = -1;


void installDefaultFiles() {
  // Make CopyIcons folders
  sceIoMkdir("ux0:data/SMLA00001", 0777);
  sceIoMkdir("ux0:data/SMLA00001/module", 0777);

  // Write default files if they don't exist
  int i;
  for (i = 0; i < (sizeof(default_files) / sizeof(DefaultFile)); i++) {
    SceIoStat stat;
    memset(&stat, 0, sizeof(stat));
    if (sceIoGetstat(default_files[i].path, &stat) < 0 || (default_files[i].replace && (int)stat.st_size != default_files[i].size))
      WriteFile(default_files[i].path, default_files[i].buffer, default_files[i].size);
  }  
}

void initLauncher() {
  installDefaultFiles();
  
  // Load modules
  int search_unk[2];
  SceUID search_modid;
  search_modid = _vshKernelSearchModuleByName("VitaShellPatch", search_unk);
  if(search_modid < 0) {
    patch_modid = taiLoadKernelModule("ux0:data/SMLA00001/module/patch.skprx", 0, NULL);
    if (patch_modid >= 0) {
      int res = taiStartKernelModule(patch_modid, 0, NULL, 0, NULL, NULL);
      if (res < 0)
        taiStopUnloadKernelModule(patch_modid, 0, NULL, 0, NULL, NULL);
    }
  }
  search_modid = _vshKernelSearchModuleByName("VitaShellKernel2", search_unk);
  if(search_modid < 0) {
    kernel_modid = taiLoadKernelModule("ux0:data/SMLA00001/module/kernel.skprx", 0, NULL);
    if (kernel_modid >= 0) {
      int res = taiStartKernelModule(kernel_modid, 0, NULL, 0, NULL, NULL);
      if (res < 0)
        taiStopUnloadKernelModule(kernel_modid, 0, NULL, 0, NULL, NULL);
    }
  }
  user_modid = sceKernelLoadStartModule("ux0:data/SMLA00001/module/user.suprx", 0, NULL, 0, NULL, NULL);

}

int WriteFile(const char *file, const void *buf, int size) {
  SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0)
    return fd;

  int written = sceIoWrite(fd, buf, size);

  sceIoClose(fd);
  return written;
}
