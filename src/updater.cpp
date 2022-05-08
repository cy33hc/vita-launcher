#include "config.h"
#include "updater.h"
#include "fs.h"
#include "sfo.h"
#include "net.h"
#include "gui.h"

char updater_message[256];

namespace Updater {
    static void fpkg_hmac(const uint8_t *data, unsigned int len, uint8_t hmac[16])
    {
        SHA1_CTX ctx;
        uint8_t sha1[20];
        uint8_t buf[64];

        sha1_init(&ctx);
        sha1_update(&ctx, data, len);
        sha1_final(&ctx, sha1);

        memset(buf, 0, 64);
        memcpy(&buf[0], &sha1[4], 8);
        memcpy(&buf[8], &sha1[4], 8);
        memcpy(&buf[16], &sha1[12], 4);
        buf[20] = sha1[16];
        buf[21] = sha1[1];
        buf[22] = sha1[2];
        buf[23] = sha1[3];
        memcpy(&buf[24], &buf[16], 8);

        sha1_init(&ctx);
        sha1_update(&ctx, buf, 64);
        sha1_final(&ctx, sha1);
        memcpy(hmac, sha1, 16);
    }

    int MakeHeadBin()
    {
        uint8_t hmac[16];
        uint32_t off;
        uint32_t len;
        uint32_t out;

        SceIoStat stat;
        memset(&stat, 0, sizeof(SceIoStat));

        if (FS::FileExists(HEAD_BIN))
            return 0;

        // Read param.sfo
        const auto sfo = FS::Load(PACKAGE_DIR "/sce_sys/param.sfo");

        // Get title id
        char titleid[12];
        memset(titleid, 0, sizeof(titleid));
        snprintf(titleid, 12, "%s", SFO::GetString(sfo.data(), sfo.size(), "TITLE_ID"));

        // Enforce TITLE_ID format
        if (strlen(titleid) != 9)
            return -1;

        // Get content id
        char contentid[48];
        memset(contentid, 0, sizeof(contentid));
        snprintf(contentid, 48, "%s", SFO::GetString(sfo.data(), sfo.size(), "CONTENT_ID"));

        // Free sfo buffer
        sfo.clear();

        // Allocate head.bin buffer
        std::vector<char> head_bin_data = FS::Load(HEAD_BIN_PATH);
        uint8_t *head_bin = malloc(head_bin_data.size());
        memcpy(head_bin, head_bin_data.data(), head_bin_data.size());

        // Write full title id
        char full_title_id[48];
        snprintf(full_title_id, sizeof(full_title_id), "EP9000-%s_00-0000000000000000", titleid);
        strncpy((char *)&head_bin[0x30], strlen(contentid) > 0 ? contentid : full_title_id, 48);

        // hmac of pkg header
        len = ntohl(*(uint32_t *)&head_bin[0xD0]);
        fpkg_hmac(&head_bin[0], len, hmac);
        memcpy(&head_bin[len], hmac, 16);

        // hmac of pkg info
        off = ntohl(*(uint32_t *)&head_bin[0x8]);
        len = ntohl(*(uint32_t *)&head_bin[0x10]);
        out = ntohl(*(uint32_t *)&head_bin[0xD4]);
        fpkg_hmac(&head_bin[off], len-64, hmac);
        memcpy(&head_bin[out], hmac, 16);

        // hmac of everything
        len = ntohl(*(uint32_t *)&head_bin[0xE8]);
        fpkg_hmac(&head_bin[0], len, hmac);
        memcpy(&head_bin[len], hmac, 16);

        // Make dir
        sceIoMkdir(PACKAGE_DIR "/sce_sys/package", 0777);

        // Write head.bin
        FS::Save(HEAD_BIN, head_bin, head_bin_data.size());

        free(head_bin);

        return 0;
    }

    int CheckAppExist(const char *titleid)
    {
        int res;
        int ret;

        ret = scePromoterUtilityCheckExist(titleid, &res);
        if (res < 0)
            return res;

        return ret >= 0;
    }

    int PromoteApp(const char *path)
    {
        int res;

        sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        res = scePromoterUtilityPromotePkgWithRif(path, 1);
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

        return res;
    }

    int InstallPackage(char *file, char *package_name)
    {
        int res;

        // Recursively clean up pkg directory
        sprintf(updater_message, "Removing temp pkg directory");
        FS::RmRecursive(PACKAGE_DIR);

        sprintf(updater_message, "Extracting vpk package");
        ExtractFile(file, PACKAGE_DIR "/", nullptr);

        // Make head.bin
        res = MakeHeadBin();
        if (res < 0)
            return res;

        // Promote app
        sprintf(updater_message, "Starting to install %s", package_name);
        res = PromoteApp(PACKAGE_DIR);
        if (res < 0)
        {
            sprintf(updater_message, "Failed to install %s", package_name);
            return res;
        }
        return 0;
    }

    void ExtractFile(char *file, char *dir, std::vector<std::string> *files_to_extract)
    {
        unz_global_info global_info;
        unz_file_info file_info;
        unzFile zipfile = unzOpen(file);
        unzGetGlobalInfo(zipfile, &global_info);
        unzGoToFirstFile(zipfile);
        uint64_t curr_extracted_bytes = 0;
        uint64_t curr_file_bytes = 0;
        int num_files = global_info.number_entry;
        char fname[512];
        char ext_fname[512];
        char read_buffer[8192];

        for (int zip_idx = 0; zip_idx < num_files; ++zip_idx)
        {
            unzGetCurrentFileInfo(zipfile, &file_info, fname, 512, NULL, 0, NULL, 0);
            sprintf(ext_fname, "%s%s", dir, fname); 
            const size_t filename_length = strlen(ext_fname);
            if (ext_fname[filename_length - 1] != '/' && ( files_to_extract == nullptr ||
                (files_to_extract != nullptr && std::find(files_to_extract->begin(), files_to_extract->end(), fname) != files_to_extract->end())))
            {
                snprintf(updater_message, 255, "Extracting %s", fname);
                curr_file_bytes = 0;
                unzOpenCurrentFile(zipfile);
                FS::MkDirs(ext_fname, true);
                FILE *f = fopen(ext_fname, "wb");
                while (curr_file_bytes < file_info.uncompressed_size)
                {
                    int rbytes = unzReadCurrentFile(zipfile, read_buffer, 8192);
                    if (rbytes > 0)
                    {
                        fwrite(read_buffer, 1, rbytes, f);
                        curr_extracted_bytes += rbytes;
                        curr_file_bytes += rbytes;
                    }
                }
                fclose(f);
                unzCloseCurrentFile(zipfile);
            }
            if ((zip_idx + 1) < num_files)
            {
                unzGoToNextFile(zipfile);
            }
        }
        unzClose(zipfile);
    }

    void StartUpdaterThread()
    {
        updater_thid = sceKernelCreateThread("updater_thread", (SceKernelThreadEntry)UpdaterThread, 0x10000100, 0x4000, 0, 0, NULL);
		if (updater_thid >= 0)
			sceKernelStartThread(updater_thid, 0, NULL);
    }

    int UpdaterThread(SceSize args, void *argp)
    {
        SceKernelFwInfo fw;
        fw.size = sizeof(SceKernelFwInfo);
        _vshSblGetSystemSwVersion(&fw);

        int itls_enso_installed = CheckAppExist(ITLS_ENSO_APP_ID);
        int vita_updated = 0;
        if (itls_enso_installed || fw.version > 0x0365000)
        {
            int ret = UpdateYoyoLauncher();
            ret = UpdateYoYoLoader();
            ret = UpdateFlycastCores();
            vita_updated = UpdateVitaLauncher();
        }

        if (!itls_enso_installed && fw.version <= 0x03650000)
        {
            sprintf(updater_message, "iTLS-Enso is not installed.\nIt's required to download icons and updates");
            sceKernelDelayThread(4000000);
        }
        if (vita_updated == 1)
        {
            sprintf(updater_message, "Vita-Laucher updated successfully.\nRestarting after 3s");
            sceKernelDelayThread(3000000);
            sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
        }
        handle_updates = false;
        Windows::SetModalMode(false);
        return sceKernelExitDeleteThread(0);
    }

    int InstallYoyoLauncher()
    {
        int ret;
        FS::MkDirs(YOYO_LAUNCHER_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking if YOYO Launcher is installed");
        ret = CheckAppExist(YOYO_LAUNCHER_ID);
        if (ret == 0)
        {
            sprintf(updater_message, "Downloading YOYO Launcher");
            ret = Net::DownloadFile(YOYO_LAUNCHER_VPK_URL, YOYO_LAUNCHER_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download YOYO Launcher vpk");
                return -1;
            }
            sprintf(updater_message, "Extracting vpk");
            FS::MkDirs(PACKAGE_DIR);
            ExtractFile(YOYO_LAUNCHER_VPK_UPDATE_PATH, PACKAGE_DIR "/", nullptr);
            InstallPackage(YOYO_LAUNCHER_VPK_UPDATE_PATH, "YOYO Launcher");
            FS::Rm(YOYO_LAUNCHER_VPK_UPDATE_PATH);
        }

        ret =  UpdateYoYoLoader();
        if (ret < 0)
            return ret;

        return 0;
    }
    
    int UpdateYoYoLoader()
    {
        std::vector<char> current_version;
        std::vector<char> update_version;
        std::vector<std::string> files;
        int current_yoyo_size;
        char git_commit[8];
        char cur_commit[8];
        int ret;

        FS::MkDirs(YOYO_LOADER_VERSION_UPDATE_PATH, true);
        sprintf(updater_message, "Checking for YoYo-Loader Update");
        if (!FS::FileExists(YOYO_LOADER_VERSION_PATH))
        {
            current_version = std::vector<char>();
            current_version.push_back(0);
        }
        else
        {
            current_version = FS::Load(YOYO_LOADER_VERSION_PATH);
            snprintf(cur_commit, 8, "%s", current_version.data());
        }
        ret = Net::DownloadFile(YOYO_LOADER_VERSION_URL, YOYO_LOADER_VERSION_UPDATE_PATH);
        if (ret < 0)
        {
            sprintf(updater_message, "Failed to check for YoYo-Loader update");
            return -1;
        }
        update_version = FS::Load(YOYO_LOADER_VERSION_UPDATE_PATH);
        update_version[update_version.size()-1] = 0;
        snprintf(git_commit, 8, strstr(update_version.data(), "\"body\":") + 10);
        files =  std::vector<std::string>();
        files.push_back("loader.bin");
        files.push_back("loader2.bin");

        if (strcmp(cur_commit, git_commit) != 0)
        {
            sprintf(updater_message, "Downloading YoYo-Loader Update");
            ret = Net::DownloadFile(YOYO_LOADER_VPK_URL, YOYO_LOADER_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download YoYo-Loader update");
                return -1;
            }

            sprintf(updater_message, "Extracting files");
            ExtractFile(YOYO_LOADER_VPK_UPDATE_PATH, "ux0:app/SMLA00002/", &files);

            FS::Save(YOYO_LOADER_VERSION_PATH, git_commit, strlen(git_commit));
            FS::Rm(YOYO_LOADER_VPK_UPDATE_PATH);
            FS::Rm(YOYO_LOADER_VERSION_UPDATE_PATH);
            return 1;
        }
        FS::Rm(YOYO_LOADER_VERSION_UPDATE_PATH);

        return 0;
    }

    int InstallAdrLauncher()
    {
        int ret;
        FS::MkDirs(ADR_LAUNCHER_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking if Adernaline Launcher is installed");
        ret = CheckAppExist(DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID);
        if (ret == 0)
        {
            sprintf(updater_message, "Downloading Adernaline Launcher");
            ret = Net::DownloadFile(ADR_LAUNCHER_VPK_URL, ADR_LAUNCHER_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download Adernaline Launcher vpk");
                return -1;
            }
            sprintf(updater_message, "Extracting Adernaline Launcher vpk");
            FS::MkDirs(PACKAGE_DIR);
            ExtractFile(ADR_LAUNCHER_VPK_UPDATE_PATH, PACKAGE_DIR "/", nullptr);
            InstallPackage(ADR_LAUNCHER_VPK_UPDATE_PATH, "Adernaline Launcher");
            FS::Rm(ADR_LAUNCHER_VPK_UPDATE_PATH);
            return 1;
        }

        return 0;
    }

    int InstallFlycastCores()
    {
        FS::MkDirs(FLYCAST_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking if Flycast is installed");
        int flycast_core_exists = FS::FileExists(FLYCAST_RETRO_ACCURACY_CORE_PATH);
        int retroarch_installed = CheckAppExist(RETROARCH_TITLE_ID);

        if (!flycast_core_exists && retroarch_installed)
        {
            sprintf(updater_message, "Downloading Flycast app");
            int ret = Net::DownloadFile(FLYCAST_VPK_URL, FLYCAST_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download Flycast vpk");
                return -1;
            }
            sprintf(updater_message, "Extracting Flycast cores to Retroarch");
            std::vector<std::string> files =  std::vector<std::string>();
            files.push_back("smc.bin");
            files.push_back("smc2.bin");

            ExtractFile(FLYCAST_VPK_UPDATE_PATH, RETROARCH_INSTALL_PATH "/", &files);

            FS::Rename(RETROARCH_INSTALL_PATH "/smc.bin", FLYCAST_RETRO_CORE_PATH);
            FS::Rename(RETROARCH_INSTALL_PATH "/smc2.bin", FLYCAST_RETRO_ACCURACY_CORE_PATH);
            FS::Rm(FLYCAST_VPK_UPDATE_PATH);

            uint64_t file_size;
            char buffer[10];
            Net::GetDownloadFileSize(FLYCAST_VPK_URL, &file_size);
            sprintf(buffer, "%d", file_size);
            FS::Save(FLYCAST_VERSION_PATH, buffer, strlen(buffer));

            return 1;
        }

        return 0;
    }

    int UpdateFlycastCores()
    {
        FS::MkDirs(FLYCAST_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking if Flycast is Updated");
        int flycast_core_exists = FS::FileExists(FLYCAST_RETRO_ACCURACY_CORE_PATH);
        int retroarch_installed = CheckAppExist(RETROARCH_TITLE_ID);
        char current_size[20];
        char new_size[20];
        uint64_t file_size;

        memset(current_size, 0, 20);
        if (FS::FileExists(FLYCAST_VERSION_PATH))
        {
            memset(current_size, 0, 20);
            void *f = FS::OpenRead(FLYCAST_VERSION_PATH);
            FS::Read(f, current_size, 20);
        }

        Net::GetDownloadFileSize(FLYCAST_VPK_URL, &file_size);
        sprintf(new_size, "%d", file_size);

        if (strcmp(current_size, new_size) != 0 && retroarch_installed)
        {
            sprintf(updater_message, "Downloading Flycast app");
            int ret = Net::DownloadFile(FLYCAST_VPK_URL, FLYCAST_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download Flycast vpk");
                return -1;
            }
            sprintf(updater_message, "Extracting Flycast cores to Retroarch");
            std::vector<std::string> files =  std::vector<std::string>();
            files.push_back("smc.bin");
            files.push_back("smc2.bin");

            ExtractFile(FLYCAST_VPK_UPDATE_PATH, RETROARCH_INSTALL_PATH "/", &files);

            FS::Rename(RETROARCH_INSTALL_PATH "/smc.bin", FLYCAST_RETRO_CORE_PATH);
            FS::Rename(RETROARCH_INSTALL_PATH "/smc2.bin", FLYCAST_RETRO_ACCURACY_CORE_PATH);
            FS::Rm(FLYCAST_VPK_UPDATE_PATH);

            FS::Save(FLYCAST_VERSION_PATH, new_size, strlen(new_size));

            return 1;
        }

        return 0;
    }

    int UpdateVitaLauncher()
    {
        std::vector<char> current_version;
        std::vector<char> update_version;
        char cur_ver[4];
        char upd_ver[4];
        int ret;

        FS::MkDirs(VITA_LAUNCHER_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking for Vita-Laucher Update");
        ret = Net::DownloadFile(VITA_LAUNCHER_VERSION_URL, VITA_LAUNCHER_VERSION_UPDATE_PATH);
        if (ret < 0)
        {
            sprintf(updater_message, "Failed to get updates");
            return -1;
        }

        current_version = FS::Load(VITA_LAUNCHER_VERSION_PATH);
        update_version = FS::Load(VITA_LAUNCHER_VERSION_UPDATE_PATH);
        snprintf(cur_ver, 4, "%s", current_version.data());
        snprintf(upd_ver, 4, "%s", update_version.data());
        ret = 0;
        if (strcmp(cur_ver, upd_ver) != 0)
        {
            sprintf(updater_message, "Downloading Vita-Laucher update");
            ret = Net::DownloadFile(VITA_LAUNCHER_VPK_URL, VITA_LAUNCHER_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download update");
                return -1;
            }

            sprintf(updater_message, "Extracting files");
            ExtractFile(VITA_LAUNCHER_VPK_UPDATE_PATH, "ux0:app/SMLA00001/", nullptr);

            FS::Save(VITA_LAUNCHER_VERSION_PATH, upd_ver, 4);
            FS::Rm(VITA_LAUNCHER_VPK_UPDATE_PATH);
            FS::Rm(VITA_LAUNCHER_VERSION_UPDATE_PATH);

            return 1;
        }
        FS::Rm(VITA_LAUNCHER_VERSION_UPDATE_PATH);

        return 0;
    }

    int UpdateYoyoLauncher()
    {
        std::vector<char> current_version;
        std::vector<char> update_version;
        char cur_ver[4];
        char upd_ver[4];
        int ret;

        FS::MkDirs(YOYO_LAUNCHER_VPK_UPDATE_PATH, true);
        sprintf(updater_message, "Checking for YOYO Laucher Update");
        ret = Net::DownloadFile(YOYO_LAUNCHER_VERSION_URL, YOYO_LAUNCHER_VERSION_UPDATE_PATH);
        if (ret < 0)
        {
            sprintf(updater_message, "Failed to get updates");
            return -1;
        }

        if (FS::FileExists(YOYO_LAUNCHER_VERSION_PATH))
        {
            current_version = FS::Load(YOYO_LAUNCHER_VERSION_PATH);
        }
        else
        {
            current_version = std::vector<char>();
            current_version.push_back(0);
        }
        update_version = FS::Load(YOYO_LAUNCHER_VERSION_UPDATE_PATH);
        snprintf(cur_ver, 4, "%s", current_version.data());
        snprintf(upd_ver, 4, "%s", update_version.data());
        if (strcmp(cur_ver, upd_ver) != 0)
        {
            sprintf(updater_message, "Downloading YOYO Laucher update");
            ret = Net::DownloadFile(YOYO_LAUNCHER_VPK_URL, YOYO_LAUNCHER_VPK_UPDATE_PATH);
            if (ret < 0)
            {
                sprintf(updater_message, "Failed to download update");
                return -1;
            }

            sprintf(updater_message, "Extracting files");
            ExtractFile(YOYO_LAUNCHER_VPK_UPDATE_PATH, "ux0:app/SMLA00002/", nullptr);

            FS::Save(YOYO_LAUNCHER_VERSION_PATH, upd_ver, strlen(upd_ver));
            FS::Rm(YOYO_LAUNCHER_VPK_UPDATE_PATH);
            ret = 1;
        }
        FS::Rm(YOYO_LAUNCHER_VERSION_UPDATE_PATH);

        return ret == 1;
    }

    void StartInstallerThread()
    {
        int itls_enso_installed = CheckAppExist(ITLS_ENSO_APP_ID);
        int adr_installed = CheckAppExist(DEFAULT_ADERNALINE_LAUNCHER_TITLE_ID);
        int yoyo_installed = CheckAppExist(YOYO_LAUNCHER_ID);
        int abm_installed = CheckAppExist(ABM_APP_ID);
        int flycast_cores_installed = FS::FileExists(FLYCAST_RETRO_ACCURACY_CORE_PATH) && CheckAppExist(RETROARCH_TITLE_ID);

        if (!adr_installed || !yoyo_installed || !abm_installed || !itls_enso_installed || !flycast_cores_installed)
        {
            handle_updates = true;
            installer_thid = sceKernelCreateThread("installer_thread", (SceKernelThreadEntry)InstallerThread, 0x10000100, 0x4000, 0, 0, NULL);
            if (installer_thid >= 0)
                sceKernelStartThread(installer_thid, 0, NULL);
        }
    }

    int InstallerThread(SceSize args, void *argp)
    {
        while (gui_mode == GUI_MODE_SCAN)
        {
            sceKernelDelayThread(100000);
        }
        sceKernelDelayThread(1500000);

        SceKernelFwInfo fw;
        fw.size = sizeof(SceKernelFwInfo);
        _vshSblGetSystemSwVersion(&fw);

        int itls_enso_installed = CheckAppExist(ITLS_ENSO_APP_ID);
        bool abm_installed = CheckAppExist(ABM_APP_ID);
        if (itls_enso_installed || fw.version > 0x03650000)
        {
            InstallAdrLauncher();
            InstallYoyoLauncher();
            InstallFlycastCores();
        }

    ERROR_EXIT:
        if (!abm_installed)
        {
            sprintf(updater_message, "Adrenaline Bubbles Manager is not installed. It is required to\nboot PSP/PS1/PSMinit games without bubbles");
            sceKernelDelayThread(4000000);
        }
        if (!itls_enso_installed && fw.version <= 0x03650000)
        {
            sprintf(updater_message, "iTLS-Enso is not installed.\nIt's required to download icons and updates");
            sceKernelDelayThread(4000000);
        }
        handle_updates = false;
        Windows::SetModalMode(false);
        return sceKernelExitDeleteThread(0);
    }
}