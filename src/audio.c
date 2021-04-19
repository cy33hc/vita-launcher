#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "vitaaudiolib.h"
#include "ogg.h"

enum Audio_FileType {
	FILE_TYPE_NONE,
	FILE_TYPE_OGG
};

typedef struct {
	int (* init)(const char *path);
	SceUInt32 (* rate)(void);
	SceUInt8 (* channels)(void);
	void (* decode)(void *buf, unsigned int length, void *userdata);
	SceUInt64 (* position)(void);
	SceUInt64 (* length)(void);
	SceUInt64 (* seek)(SceUInt64 index);
	void (* term)(void);
} Audio_Decoder;

static enum Audio_FileType file_type = FILE_TYPE_NONE;
Audio_Metadata metadata = {0};
static Audio_Metadata empty_metadata = {0};
static Audio_Decoder decoder = {0}, empty_decoder = {0};
SceBool playing = SCE_TRUE, m_paused = SCE_FALSE;

static void Audio_Decode(void *buf, unsigned int length, void *userdata) {
	if (playing == SCE_FALSE || m_paused == SCE_TRUE) {
		short *buf_short = (short *)buf;
		unsigned int count;
		for (count = 0; count < length * 2; count++)
			*(buf_short + count) = 0;
	} 
	else
		(* decoder.decode)(buf, length, userdata);
}

int Audio_Init(const char *path) {
	playing = SCE_TRUE;
	m_paused = SCE_FALSE;
	
	file_type = FILE_TYPE_OGG;
	decoder.init = OGG_Init;
	decoder.rate = OGG_GetSampleRate;
	decoder.channels = OGG_GetChannels;
	decoder.decode = OGG_Decode;
	decoder.position = OGG_GetPosition;
	decoder.length = OGG_GetLength;
	decoder.seek = OGG_Seek;
	decoder.term = OGG_Term;

	(* decoder.init)(path);
	vitaAudioInit((* decoder.rate)(), (* decoder.channels)() == 2? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO);
	vitaAudioSetChannelCallback(0, Audio_Decode, NULL);
	return 0;
}

SceBool Audio_IsPaused(void) {
	return m_paused;
}

void Audio_Pause(void) {
	m_paused = !m_paused;
}

void Audio_Stop(void) {
	playing = !playing;
}

SceUInt64 Audio_GetPosition(void) {
	return (* decoder.position)();
}

SceUInt64 Audio_GetLength(void) {
	return (* decoder.length)();
}

SceUInt64 Audio_GetPositionSeconds(void) {
	return (Audio_GetPosition() / (* decoder.rate)());
}

SceUInt64 Audio_GetLengthSeconds(void) {
	return (Audio_GetLength() / (* decoder.rate)());
}

SceUInt64 Audio_Seek(SceUInt64 index) {
	return (* decoder.seek)(index);
}

void Audio_Term(void) {
	playing = SCE_TRUE;
	m_paused = SCE_FALSE;

	vitaAudioSetChannelCallback(0, NULL, NULL); // Clear channel callback
	vitaAudioEndPre();
	sceKernelDelayThread(100 * 1000);
	vitaAudioEnd();
	(* decoder.term)();

	// Clear metadata struct
	metadata = empty_metadata;
	decoder = empty_decoder;
}
