#ifndef _ELEVENMPV_AUDIO_LIB_H_
#define _ELEVENMPV_AUDIO_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <psp2/audioout.h>

#define VITA_NUM_AUDIO_CHANNELS 1
#define VITA_NUM_AUDIO_SAMPLES 960

typedef void (* vitaAudioCallback_t)(void *stream, unsigned int length, void *userdata);

typedef struct {
	int threadhandle;
	int handle;
	int volumeleft;
	int volumeright;
	vitaAudioCallback_t callback;
	void *userdata;
} VITA_audio_channelinfo;

typedef int (* vitaAudioThreadfunc_t)(int args, void *argp);

void vitaAudioSetVolume(int channel, int left, int right);
void vitaAudioSetChannelCallback(int channel, vitaAudioCallback_t callback, void *userdata);
int vitaAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, const void *buf);
int vitaAudioInit(int frequency, SceAudioOutMode mode);
void vitaAudioEndPre(void);
void vitaAudioEnd(void);

#ifdef __cplusplus
}
#endif

#endif
