#ifndef __UPDATER_H__
#define __UPDATER_H__
#include <vitasdk.h>
#include "zip.h"
#include "unzip.h"
#include "windows.h"

extern "C" {
    #include "sha1.h"
}

#define ntohl __builtin_bswap32

#define HEAD_BIN_PATH "ux0:app/SMLA00001/head.bin"
#define PACKAGE_DIR "ux0:data/SMLA00001/tmp/pkg"
#define HEAD_BIN PACKAGE_DIR "/sce_sys/package/head.bin"

typedef struct {
  char *file;
} InstallArguments;

extern char updater_message[];
static SceUID updater_thid = -1;
static SceUID installer_thid = -1;

namespace Updater {
    int PromoteApp(const char *path);
    int CheckAppExist(const char *titleid);

    int MakeHeadBin();

    int InstallPackage(char *file);
    void StartUpdaterThread();
    int UpdaterThread(SceSize args, void *argp);
    void ExtractFile(char *file, char *dir, std::vector<std::string> *files_to_extract);
    int InstallYoyoLauncher();
    int InstallAdrLauncher();
    int UpdateYoyoLauncher();
    int UpdateYoYoLoader();
    int UpdateVitaLauncher();
    void StartInstallerThread();
    int InstallerThread(SceSize args, void *argp);
}
#endif
