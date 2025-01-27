#include "../platform.h"
#include <zvuk.h>

void set_mixing_device(int mix_dev,int mix_freq,...) {

}
char start_mixing() {
    return 0;
}
void stop_mixing() {

}
void play_sample(int channel,void *sample,int32_t size,int32_t lstart,int32_t sfreq,int type) {

}
void set_channel_volume(int channel,int left,int right) {

}
void set_end_of_song_callback(const char * (*cb)(void *), void *ctx) {

}

void fade_music() {

}
int mix_back_sound(int synchro) {
    return 0;
}

//int open_backsound(char *filename);
void change_music(const char *filename) {

}

//char *device_name(int device);
//void force_music_volume(int volume);

//void set_backsnd_freq(int freq);

char get_channel_state(int channel) {
    return 0;
}
void get_channel_volume(int channel,int *left,int *right) {

}
void mute_channel(int channel) {

}
void chan_break_loop(int channel) {

}
void chan_break_ext(int channel,void *org_sample,int32_t size_sample) {

}

char set_snd_effect(int funct,int data) {
    return 0;
}
char check_snd_effect(int funct) {
    return 0;
}
int  get_snd_effect(int funct) {
    return 0;
}

void *PrepareVideoSound(int mixfreq, int buffsize) {
    return 0;
}
char LoadNextVideoFrame(void *buffer, char *data, int size, short *xlat, short *accnums, int32_t *writepos) {
    return 0;

}
void DoneVideoSound(void *buffer) {

}

const char *device_name(int )
  {
  return "SDL sound device";
  }



