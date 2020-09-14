#include <cstring>
#include "sfo.h"

#include <stdexcept>

static constexpr uint32_t SFO_MAGIC = 0x46535000;

namespace SFO {
    const char* GetString(const char* buffer, size_t size, const char *name)
    {
        if (size < sizeof(SfoHeader))
            throw std::runtime_error("truncated param.sfo");

        const SfoHeader* header = reinterpret_cast<const SfoHeader*>(buffer);
        const SfoEntry* entries =
                reinterpret_cast<const SfoEntry*>(buffer + sizeof(SfoHeader));

        if (header->magic != SFO_MAGIC)
            throw std::runtime_error("can't parse SFO, invalid magic");

        if (size < sizeof(SfoHeader) + header->count * sizeof(SfoEntry))
            throw std::runtime_error("truncated param.sfo");

        for (uint32_t i = 0; i < header->count; i++) {
            const char* key = reinterpret_cast<const char*>(buffer + header->keyofs + entries[i].nameofs);
            if (strcmp(key, name) == 0)
                return reinterpret_cast<const char*>(buffer + header->valofs + entries[i].dataofs);
        }
        
        return {};
    }
}