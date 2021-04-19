#ifndef _ELEVENMPV_AUDIO_H_
#define _ELEVENMPV_AUDIO_H_

#include <psp2/types.h>
#include <vita2d.h>

#define R_FAILED(res)      ((res)<0)

extern SceBool playing, paused;

typedef struct {
	SceBool has_meta;
    char title[64];
    char album[64];
    char artist[64];
    char year[64];
    char comment[64];
    char genre[64];
    vita2d_texture *cover_image;
} Audio_Metadata;

extern Audio_Metadata metadata;

int Audio_Init(const char *path);
SceBool Audio_IsPaused(void);
void Audio_Pause(void);
void Audio_Stop(void);
SceUInt64 Audio_GetPosition(void);
SceUInt64 Audio_GetLength(void);
SceUInt64 Audio_GetPositionSeconds(void);
SceUInt64 Audio_GetLengthSeconds(void);
SceUInt64 Audio_Seek(SceUInt64 index);
void Audio_Term(void);

#endif
