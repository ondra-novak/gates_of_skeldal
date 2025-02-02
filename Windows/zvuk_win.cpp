#include <skeldal_win.h>
#include <platform/sound.h>

extern "C" 
  {
  
int mix_back_sound(int synchro)
  {

  return 0;
  }

int get_timer_value()
  {
  return GetTickCount()/TIMERSPEED;
  }


int get_snd_effect(int funct)
  {
  return 0;
  }

char set_snd_effect(int funct,int data)
  {
  return 0;
  }

char check_snd_effect(int funct)
  {
  return 0;
  }

void change_music(char *filename)
  {

  }


void set_backsnd_freq(int freq)
  {

  }

void fade_music()
  {

  }

void set_mixing_device(int mix_dev,int mix_freq,...)
  {
  }
char start_mixing()
  {
  return 0;
  }
void stop_mixing()
  {
  }
void play_sample(int channel,void *sample,int32_t size,int32_t lstart,int32_t sfreq,int type)
  {
  }
void set_channel_volume(int channel,int left,int right)
  {
  }
int open_backsound(char *filename)
  {
  return 0;
  }
char *device_name(int device)
  {
  return "";
  }
void force_music_volume(int volume)
  {
  }
char get_channel_state(int channel)
  {
  return 0;
  }
void get_channel_volume(int channel,int *left,int *right)
  {
  }
void mute_channel(int channel)
  {
  }
void chan_break_loop(int channel)
  {
  }
void chan_break_ext(int channel,void *org_sample,int32_t size_sample) //zrusi loop s moznosti dohrat zvuk
  {
  }

  }

