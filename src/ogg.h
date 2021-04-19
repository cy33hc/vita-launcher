#ifndef _ELEVENMPV_AUDIO_OGG_H_
#define _ELEVENMPV_AUDIO_OGG_H_

#include <psp2/types.h>

int OGG_Init(const char *path);
SceUInt32 OGG_GetSampleRate(void);
SceUInt8 OGG_GetChannels(void);
void OGG_Decode(void *buf, unsigned int length, void *userdata);
SceUInt64 OGG_GetPosition(void);
SceUInt64 OGG_GetLength(void);
SceUInt64 OGG_Seek(SceUInt64 index);
void OGG_Term(void);

#endif
