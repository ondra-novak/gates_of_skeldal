#pragma once

#include "../libs/music.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {

SND_PING,
SND_GVOLUME,
SND_BASS,
SND_TREBL,
SND_SWAP,
SND_LSWAP,
SND_SURROUND,
SND_OUTFILTER,
SND_GFX,
SND_MUSIC,
SND_XBASS,

SND_MAXFUNCT} AUDIO_PROPERTY;

void game_sound_init_device(const INI_CONFIG_SECTION *audio_section);


typedef char (*TEND_OF_SONG_CALLBACK)(void *ctx, TMUSIC_SOURCE *, TMUSIC_SOURCE_TYPE *);


char start_mixing();
void stop_mixing();
void play_sample(int channel,const void *sample,int32_t size,int32_t lstart,int32_t sfreq,int type);
void set_channel_volume(int channel,int left,int right);
void set_end_of_song_callback(TEND_OF_SONG_CALLBACK cb, void *ctx);
void play_music(const TMUSIC_SOURCE *source, TMUSIC_SOURCE_TYPE type);
void stop_play_music();
void fade_music();
int mix_back_sound(int synchro);
char get_channel_state(int channel);
void get_channel_volume(int channel,int *left,int *right);
void mute_channel(int channel);
void chan_break_loop(int channel);
void chan_break_ext(int channel,const void *org_sample,int32_t size_sample);
char set_snd_effect(AUDIO_PROPERTY funct,int data);
char check_snd_effect(AUDIO_PROPERTY funct);
int  get_snd_effect(AUDIO_PROPERTY funct);
void *PrepareVideoSound(int mixfreq, int buffsize) ;
char LoadNextVideoFrame(void *buffer, const char *data, int size, const short *xlat, short *accnums, int32_t *writepos);
void DoneVideoSound(void *buffer);
const char *device_name(int );


#ifdef __cplusplus
}
#endif
