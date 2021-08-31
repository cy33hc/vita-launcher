#include "fs.h"

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> 
#include <sys/stat.h>

#include <algorithm>

namespace FS {
    void MkDirs(const std::string& ppath)
    {
        std::string path = ppath;
        path.push_back('/');
        auto ptr = path.begin();
        while (true)
        {
            ptr = std::find(ptr, path.end(), '/');
            if (ptr == path.end())
                break;

            char last = *ptr;
            *ptr = 0;
            #ifdef _WIN64
                int err = mkdir(path.c_str());
            #else
                int err = mkdir(path.c_str(), 0777);
            #endif
            *ptr = last;
            ++ptr;
        }
    }

    void Rm(const std::string& file)
    {
        int err = remove(file.c_str());
    }

    int64_t GetSize(const std::string& path)
    {
        struct stat stats;
        int err = stat(path.c_str(), &stats);
        if (err < 0)
        {
            return -1;
        }
        return stats.st_size;
    }

    bool FileExists(const std::string& path)
    {
        struct stat stats;
        return stat(path.c_str(), &stats) >= 0;
    }

    bool FolderExists(const std::string& path)
    {
        struct stat stats;
        stat(path.c_str(), &stats);
        return stats.st_mode & S_IFDIR;
    }

    void Rename(const std::string& from, const std::string& to)
    {
        // try to remove first because sceIoRename does not overwrite
        remove(to.c_str());
        int res = rename(from.c_str(), to.c_str());
    }

    void* Create(const std::string& path)
    {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);

        return (void*)(intptr_t)fd;
    }

    void* OpenRW(const std::string& path)
    {
        int fd = open(path.c_str(), O_RDWR, 0777);
        return (void*)(intptr_t)fd;
    }

    void* OpenRead(const std::string& path)
    {
        int fd = open(path.c_str(), O_RDONLY, 0777);
        return (void*)(intptr_t)fd;
    }

    void* Append(const std::string& path)
    {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0777);
        return (void*)(intptr_t)fd;
    }

    int64_t Seek(void* f, uint64_t offset)
    {
        auto const pos = lseek((intptr_t)f, offset, SEEK_SET);
        return pos;
    }

    int Read(void* f, void* buffer, uint32_t size)
    {
        const auto ret = read((int)(intptr_t)f, buffer, size);
        return ret;
    }

    int Write(void* f, const void* buffer, uint32_t size)
    {
        int ret = write((int)(intptr_t)f, buffer, size);
        return ret;
    }

    void Close(void* f)
    {
        int fd = (int)(intptr_t)f;
        int err = close(fd);
    }

    std::vector<char> Load(const std::string& path)
    {
        int fd = open(path.c_str(), O_RDONLY, 0777);
        if (fd < 0)
            return std::vector<char>(0);

        const auto size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        std::vector<char> data(size);

        const auto ret = read(fd, data.data(), data.size());
        close(fd);
        if (ret < 0)
            return std::vector<char>(0);

        data.resize(ret);

        return data;
    }

    void Save(const std::string& path, const void* data, uint32_t size)
    {
        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (fd < 0)
            return;

        const char* data8 = static_cast<const char*>(data);
        while (size != 0)
        {
            int written = write(fd, data8, size);
            close(fd);
            if (written <= 0)
                return;
            data8 += written;
            size -= written;
        }
    }

    std::vector<std::string> ListDir(const std::string& path)
    {
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
            return std::vector<std::string>(0);

        std::vector<std::string> out;
        struct dirent *dent;
        while (true)
        {
            dent = readdir(dir);
            if (dent == nullptr) {
                closedir(dir);
                return out;
            }
            else
                out.push_back(dent->d_name);
        }
        closedir(dir);

        return out;
    }

    std::vector<std::string> ListFiles(const std::string& path)
    {
        DIR *dir = opendir(path.c_str());
        if (dir == nullptr)
            return std::vector<std::string>(0);

        std::vector<std::string> out;
        struct dirent *dent;
        struct stat stats;
        while (true)
        {
            dent = readdir(dir);
            if (dent == nullptr) {
                closedir(dir);
                return out;
            }
            std::string full_path = path + "/" + dent->d_name;
            #ifdef _WIN64
                stat(full_path.c_str(), &stats);
            #else
                lstat(full_path.c_str(), &stats);
            #endif
            if (S_ISDIR(stats.st_mode))
            {
                if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0 )
                {
                    std::vector<std::string> files = FS::ListFiles(path + "/" + dent->d_name);
                    for (std::vector<std::string>::iterator it=files.begin(); it!=files.end(); )
                    {
                        out.push_back(std::string(dent->d_name) + "/" + *it);
                        ++it;
                    }
                }
            }
            else
            {
                out.push_back(dent->d_name);
            }
        }
        closedir(dir);
        return out;
    }

}
