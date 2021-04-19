#include <psp2/kernel/threadmgr.h>
#include <string.h>

#include "vitaaudiolib.h"

static int audio_ready = 0;
static short vitaAudioSoundBuffer[VITA_NUM_AUDIO_CHANNELS][2][VITA_NUM_AUDIO_SAMPLES][2];
static VITA_audio_channelinfo vitaAudioStatus[VITA_NUM_AUDIO_CHANNELS];
static volatile int audio_terminate = 0;

void vitaAudioSetVolume(int channel, int left, int right) {
	vitaAudioStatus[channel].volumeleft = left;
	vitaAudioStatus[channel].volumeright = right;
}

void vitaAudioSetChannelCallback(int channel, vitaAudioCallback_t callback, void *userdata) {
	volatile VITA_audio_channelinfo *pci = &vitaAudioStatus[channel];

	if (callback == 0)
		pci->callback = 0;
	else
		pci->callback = callback;
}

int vitaAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, const void *buf) {
	if (!audio_ready)
		return(-1);

	if (channel >= VITA_NUM_AUDIO_CHANNELS)
		return(-1);

	if (vol1 > SCE_AUDIO_OUT_MAX_VOL)
		vol1 = SCE_AUDIO_OUT_MAX_VOL;

	if (vol2 > SCE_AUDIO_OUT_MAX_VOL)
		vol2 = SCE_AUDIO_OUT_MAX_VOL;

	int vol[2] = {vol1, vol2};
	sceAudioOutSetVolume(vitaAudioStatus[channel].handle, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
	return sceAudioOutOutput(vitaAudioStatus[channel].handle, buf);
}

static int vitaAudioChannelThread(unsigned int args, void *argp) {
	volatile int bufidx = 0;

	int channel = *(int *) argp;

	while (audio_terminate == 0) {
		void *bufptr = &vitaAudioSoundBuffer[channel][bufidx];
		vitaAudioCallback_t callback;
		callback = vitaAudioStatus[channel].callback;

		if (callback)
			callback(bufptr, VITA_NUM_AUDIO_SAMPLES, vitaAudioStatus[channel].userdata);
		else {
			unsigned int *ptr = bufptr;
			int i;
			for (i = 0; i < VITA_NUM_AUDIO_SAMPLES; ++i)
				*(ptr++) = 0;
		}

		vitaAudioOutBlocking(channel, vitaAudioStatus[channel].volumeleft, vitaAudioStatus[channel].volumeright, bufptr);
		bufidx = (bufidx ? 0:1);
	}

	sceKernelExitThread(0);
	return(0);
}

int vitaAudioInit(int frequency, SceAudioOutMode mode) {
	int i, ret;
	int failed = 0;
	char str[32];

	audio_terminate = 0;
	audio_ready = 0;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		vitaAudioStatus[i].handle = -1;
		vitaAudioStatus[i].threadhandle = -1;
		vitaAudioStatus[i].volumeright = SCE_AUDIO_OUT_MAX_VOL;
		vitaAudioStatus[i].volumeleft  = SCE_AUDIO_OUT_MAX_VOL;
		vitaAudioStatus[i].callback = 0;
		vitaAudioStatus[i].userdata = 0;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if ((vitaAudioStatus[i].handle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, VITA_NUM_AUDIO_SAMPLES, frequency, mode)) < 0)
			failed = 1;
	}

	if (failed) {
		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
			if (vitaAudioStatus[i].handle != -1)
				sceAudioOutReleasePort(vitaAudioStatus[i].handle);

			vitaAudioStatus[i].handle = -1;
		}

		return 0;
	}

	audio_ready = 1;
	strcpy(str, "audiot0");

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		str[6]= '0' + i;
		vitaAudioStatus[i].threadhandle = sceKernelCreateThread(str, (SceKernelThreadEntry)&vitaAudioChannelThread, 0x40, 0x10000, 0, 0, NULL);

		if (vitaAudioStatus[i].threadhandle < 0) {
			vitaAudioStatus[i].threadhandle = -1;
			failed = 1;
			break;
		}

		ret = sceKernelStartThread(vitaAudioStatus[i].threadhandle, sizeof(i), &i);

		if (ret != 0) {
			failed = 1;
			break;
		}
	}

	if (failed) {
		audio_terminate = 1;

		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
			if (vitaAudioStatus[i].threadhandle != -1)
				sceKernelDeleteThread(vitaAudioStatus[i].threadhandle);

			vitaAudioStatus[i].threadhandle = -1;
		}

		audio_ready = 0;
		return 0;
	}

	return 1;
}

void vitaAudioEndPre(void) {
	audio_ready = 0;
	audio_terminate = 1;
}

void vitaAudioEnd(void) {
	int i = 0;
	audio_ready = 0;
	audio_terminate = 1;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if (vitaAudioStatus[i].threadhandle != -1)
			sceKernelDeleteThread(vitaAudioStatus[i].threadhandle);

		vitaAudioStatus[i].threadhandle = -1;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if (vitaAudioStatus[i].handle != -1) {
			sceAudioOutReleasePort(vitaAudioStatus[i].handle);
			vitaAudioStatus[i].handle = -1;
		}
	}
}
