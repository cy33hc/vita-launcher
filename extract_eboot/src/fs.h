#ifndef LAUNCHER_FS_H
#define LAUNCHER_FS_H

#pragma once

#include <string>
#include <vector>
#include <unistd.h>

namespace FS {
    void MkDirs(const std::string& path);
    void Rm(const std::string& file);
    void RmDir(const std::string& path);
    int64_t GetSize(const char* path);

    bool FileExists(const std::string& path);
    bool FolderExists(const std::string& path);
    
    void Rename(const std::string& from, const std::string& to);

    // creates file (if it exists, truncates size to 0)
    int Create(const std::string& path);

    // open existing file in read/write, fails if file does not exist
    int OpenRW(const std::string& path);
 
    // open existing file in read/write, fails if file does not exist
    int OpenRead(const std::string& path);

    // open file for writing, next write will append data to end of it
    int Append(const std::string& path);

    void Close(int fd);

    off_t Seek(int fd, off_t offset);
    ssize_t Read(int fd, void* buffer, size_t size);
    ssize_t Write(int fd, const void* buffer, size_t size);

    std::vector<char> Load(const std::string& path);
    void Save(const std::string& path, const void* data, uint32_t size);

    std::vector<std::string> ListDir(const std::string& path);
    std::vector<std::string> ListFiles(const std::string& path);
}

#endif