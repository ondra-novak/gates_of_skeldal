#include <skeldal_win.h>
#include <bgraph.h>
#include <bgraph2dx.h>
#include <stdio.h>
#include "types.h"
#include "memman.h"
#include "mem.h"
#include "mgifmem.h"
#include <zvuk.h>

static HANDLE mapped_mgif;
static HANDLE mgif_file;
static MGIF_HEADER_T *mgif_header;

static short mgif_accnums[2];
static long mgif_writepos;

static void *OpenMGFFile(const char *filename)
  {
  mgif_file=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  if (mgif_file==INVALID_HANDLE_VALUE) return NULL;
  mapped_mgif=CreateFileMapping(mgif_file,NULL,PAGE_READONLY,0,0,NULL);
  if (mapped_mgif==INVALID_HANDLE_VALUE) return NULL;
  return MapViewOfFile(mapped_mgif,FILE_MAP_READ,0,0,0);
  }

static word *paleta;

static word *picture;
static word *anim_render_buffer;
static void *sound;

static void StretchImageHQ(word *src, word *trg, unsigned long linelen, char full)
  {
  word xs=src[0],ys=src[1];
  word *s,*t;
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
  }

static void PlayMGFFile(void *file, MGIF_PROC proc,int ypos,char full)
  {
  mgif_install_proc(proc);
  sound=PrepareVideoSound(22050,256*1024);
  mgif_accnums[0]=mgif_accnums[1]=0;
  mgif_writepos=65536;
  picture=getmem(2*3+320*180*2);
  picture[0]=320;
  picture[1]=180;
  picture[2]=15;
  memset(picture+3,0,320*180*2);
  anim_render_buffer=picture+3;
  mgif_header=(MGIF_HEADER_T *)file;
  file=open_mgif(file);
  if (file==NULL) return;
  while (file) 
	{
    __try
	  {
	  file=mgif_play(file);
	  }
  __except(1)
  	  {
	  SEND_LOG("(PLAYANIM) Exception raised",0,0);
	  file=NULL;
	  }
	StretchImageHQ(picture, GetScreenAdr()+ypos*scr_linelen2, scr_linelen2,full);
	showview(0,ypos,0,360);
	if (_bios_keybrd(_KEYBRD_READY)==0) mix_back_sound(0);
	else 
	  {
	  _bios_keybrd(_KEYBRD_READ);
	  break;
	  }
	}
  close_mgif();  
  DoneVideoSound(sound);
  free(picture);
  }

static void CloseMGFFile(void *file)
  {
  UnmapViewOfFile(file);
  CloseHandle(mapped_mgif);
  CloseHandle(mgif_file);
  }

void show_full_lfb12e(void *target,void *buff,void *paleta);
void show_delta_lfb12e(void *target,void *buff,void *paleta);
void show_delta_lfb12e_dx(void *target,void *buff,void *paleta);
void show_full_lfb12e_dx(void *target,void *buff,void *paleta);


void BigPlayProc(int act,void *data,int csize)
  {
  switch (act)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_DELTA:show_delta_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_PAL:paleta=data;break;
	 case MGIF_SOUND: 
	   while (LoadNextVideoFrame(sound,data,csize,mgif_header->ampl_table,mgif_accnums,&mgif_writepos)==0);
     }
  }

void play_animation(char *filename,char mode,int posy,char sound)
  {
  void *mgf=OpenMGFFile(filename);
  change_music(NULL);
  if (mgf==NULL) return;
  PlayMGFFile(mgf,BigPlayProc,posy,mode & 0x80);
  CloseMGFFile(mgf);
  }

void set_title_list(char **titles)
  {

  }
void set_play_attribs(void *screen,char rdraw,char bm,char colr64)
  {

  }
