#include <platform/platform.h>
#include <libs/bgraph.h>
#include <stdio.h>
#include "types.h"
#include "memman.h"
#include "mem.h"
#include "mgifmem.h"
#include <platform/sound.h>





static const word *paleta;

static word *picture;
static word *anim_render_buffer;
static void *sound;

static inline word avg_pixels(word a, word b) {
    return ((a & 0x7BDE)+(b & 0x7BDE)) >> 1;
}

static void StretchImageHQ(word *src, word *trg, int32_t linelen, char full)
  {
  word xs=src[0],ys=src[1];

  word *src_row = src+3;
  word *trg_row = trg;

  for(int y = 0; y < ys; ++y) {

      for (int x = 0; x < xs; ++x) {
          word n = src_row[x];
          trg_row[2*x] = n;
      }

      trg_row += linelen;

/*
      if (y+1 < ys ) {
          for (int x = 0; x < xs; ++x) {
              word n1 = src_row[x];
              word n2 = src_row[x+xs];
              word n3 = y > 0?src_row[x-xs]:n1;
              word n4 = y < (ys-2)?src_row[x+2*xs]:n2;
              word n5 = avg_pixels(n1, n2);
              word n6 = avg_pixels(n3, n4);
              word n7 = avg_pixels(n5, n6);
              trg_row[2*x] = n7;
              trg_row[2*x+1] = n7;
          }
      }
*/
      trg_row += linelen;

      src_row += xs;
  }

  trg_row = trg;

  for (int y = 0; y < ys; ++y) {
      for (int x = 0; x < xs-1; ++x) {
          word n1 = trg_row[2*x];
          word n2 = trg_row[2*x+2];
          word n3 = x > 0?trg_row[2*x-2]:n1;
          word n4 = x < (xs-2)?trg_row[2*x+2]:n2;
          word n5 = avg_pixels(n1, n2);
          word n6 = avg_pixels(n3, n4);
          word n7 = avg_pixels(n5, n6);
          trg_row[2*x+1] = n7;
      }
      trg_row += 2*linelen;
  }

  trg_row = trg;

  for (int y = 0; y < ys-1; ++y) {
      for (int x = 0; x < 2*xs; ++x) {
          word n1 = trg_row[x];
          word n2 = trg_row[x+2*linelen];
          word n3 = y > 1?trg_row[x-2*linelen]:n1;
          word n4 = y < (ys-2)?trg_row[x+4*linelen]:n2;
          word n5 = avg_pixels(n1, n2);
          word n6 = avg_pixels(n3, n4);
          word n7 = avg_pixels(n5, n6);
          trg_row[x+linelen] = avg_pixels(n7,0);
      }
      trg_row += 2*linelen;
  }

#if 0
  int x,y;
  src+=3;
  for (y=0,s=src,t=trg;y<ys;y++,t+=linelen*2,s+=xs)
	for (x=0;x<xs;x++)
	  {
	  word val;
	  t[x*2]=s[x]+(s[x]&0x7fe0);
	  if (x)
		{
		val=((s[x-1] & 0x7bde)+(s[x] & 0x7bde))>>1;
		t[x*2-1]=val+(val&0x7fe0);
		}
	  if (full)
		{
		if (y)
		  {
		  val=((s[x-xs] & 0x7bde)+(s[x] & 0x7bde))>>1;
		  t[x*2-linelen]=val+(val&0x7fe0);
		  }
		if (x && y)
		  {
		  val=((s[x-xs-1] & 0x7bde)+(s[x] & 0x7bde))>>1;
		  t[x*2-linelen-1]=val+(val&0x7fe0);
		  }
		}
	  }
#endif
  }



static void PlayMGFFile(const void *file, MGIF_PROC proc,int ypos,char full)
  {
  int32_t scr_linelen2 = GetScreenPitch();
  mgif_install_proc(proc);
  sound=PrepareVideoSound(22050,256*1024);
  picture=getmem(2*3+320*180*2);
  picture[0]=320;
  picture[1]=180;
  picture[2]=15;
  memset(picture+3,0,320*180*2);
  anim_render_buffer=picture+3;
  file=open_mgif(file);
  if (file==NULL) return;
  MGIF_HEADER_T *hdr =(MGIF_HEADER_T *)file;
  hdr->accnums[0] = hdr->accnums[1] = 0;
  hdr->sound_write_pos = 65536;
  char f = 1;
  while (f)
	{
      f=mgif_play(file);
      StretchImageHQ(picture, GetScreenAdr()+ypos*scr_linelen2, scr_linelen2,full);
      showview(0,ypos,0,360);
      if (game_display_is_quit_requested()) {
          break;
      } else if (_bios_keybrd(_KEYBRD_READY)) {
          _bios_keybrd(_KEYBRD_READ);
          break;
	  }
	}
  close_mgif(file);
  DoneVideoSound(sound);
  free(picture);
  }


void show_full_lfb12e(void *target,const void *buff,const void *paleta);
void show_delta_lfb12e(void *target,const void *buff,const void *paleta);
void show_delta_lfb12e_dx(void *target,void *buff,void *paleta);
void show_full_lfb12e_dx(void *target,void *buff,void *paleta);




void BigPlayProc(MGIF_HEADER_T *hdr,int act,const void *data,int csize)
  {
  switch (act)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_DELTA:show_delta_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_PAL:paleta=data;break;
	 case MGIF_SOUND:
	   while (LoadNextVideoFrame(sound,data,csize,hdr->ampl_table,hdr->accnums,&hdr->sound_write_pos)==0);
     }
  }

void play_animation(const char *filename,char mode,int posy,char sound)
  {
  size_t sz;
  void *mgf=map_file_to_memory(file_icase_find(filename), &sz);
  change_music(NULL);
  if (mgf==NULL) return;
  PlayMGFFile(mgf,BigPlayProc,posy,mode & 0x80);
  unmap_file(mgf, sz);
  }

void set_title_list(char **titles)
  {

  }
void set_play_attribs(void *screen,char rdraw,char bm,char colr64)
  {

  }
