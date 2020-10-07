#ifndef LAUNCHER_FS_H
#define LAUNCHER_FS_H

#pragma once

#include <string>
#include <vector>

#include <cstdint>

namespace FS {
    void MkDirs(const std::string& path);
    void Rm(const std::string& file);
    void RmDir(const std::string& path);
    int64_t GetSize(const char* path);

    bool FileExists(const std::string& path);
    bool FolderExists(const std::string& path);
    
    void Rename(const std::string& from, const std::string& to);

    // creates file (if it exists, truncates size to 0)
    void* Create(const std::string& path);
    // open existing file in read/write, fails if file does not exist
    void* OpenRW(const std::string& path);
    // open file for writing, next write will append data to end of it
    void* Append(const std::string& path);

    void Close(void* f);

    int64_t Seek(void* f, uint64_t offset);
    int Read(void* f, void* buffer, uint32_t size);
    int Write(void* f, const void* buffer, uint32_t size);

    std::vector<char> Load(const std::string& path);
    void Save(const std::string& path, const void* data, uint32_t size);

    std::vector<std::string> ListDir(const std::string& path);

}

#endif