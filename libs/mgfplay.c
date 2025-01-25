#include <skeldal_win.h>
// MOTION GIF - LZW komprimovana animace v rozliseni 320x180 256 barev avsak
// upravena pro prehravani v HICOLOR pro konkretni rezim (32768 barev)


// Format
  /*

     [ FRAME ] <pocet_chunku 1b><delka frame 3b>
        [CHUNK] <cislo_akce 1b><delka_chunku 3b> !pozor delka je 3 bajtova (tj 16MB)
                    0  - PRADNY CHUNK
                    1  - LZW FULL SCREEN
                    2  - MGIF DELTA
                    3  - HICOLOR PALETE
                    4  - SOUND TRACK
                    5  - TEXT TRACK
                    6  - MGIF COPY
                    7  - SOUND INIT

   Popis Chunku:
     PRAZDNY_CHUNK - muze obsahovat soukrome informace

     LZW_FULL_SCREEN
                 - ihned za hlavickou nasleduje LZW komprimovany obrazek
                   v rozliseni 320x180 256 barev

     MGIF_DELTA
                 - cely blok je LZW komprimovany. Po dekomprimace blok obsahuje
                   dva druhy informaci. Prvni DWORD je ofset od pocatku dat
                   ktery ukazuje na zacatek graficke informace. Po tomto
                   DWORDU jsou ulozeny data o umisteni grafickych bodu v obrazu
                   Prvni byte znaci, kolik je nutne preskocit slov. (tj *2 byte)
                   Dalsi byte znaci, kolik je nutne zapsat slov (tj *2 byte)
                   Po tomto bytu je nutne prenest presne tolik slov z
                   grafickeho bloku na obrazovku. Dvojice bajtu se opakuji
                   dokud ani jeden z nich nema nejvyssi 2 bity nastavene.
                   Pak tento byte znaci ze uz na radce nic vic neni. Dolni cast
                   byte (tj 6 bitu) pak udava pocet radku, ktere je treba pre-
                   skocit. (nula=zadny).

     HICOLOR PALETTE
                 - Prvni byte znamena pocet aktivnich barev. Pak nasleduje
                   paleta ve formatu xRRRRRGGGGGBBBBB. Rozdil oproti vsem
                   beznym paletam je v tom ze neni treba v palete udrzovat
                   barvy, ktere uz na obrazovce jsou, protoze zmena palety
                   se neprojevi na jiz zobrazenem obrazku.

     SOUND_TRACK
                 - Kazdy frame obsahuje zvukovou stopu. Je to RAW format.
                   Pro verzi MGIF97 zde je zvukova informace ulozena ve
                   zvukove kvalite 16-bit stereo 22000Khz, ovsem komprimovana
                   na 50% jako MUS.
     TEXT_TRACK
                 - Navic je ve mozne ve frame umistit textovou zpravu (titulek)
                   Prvni WORD udava dobu zobrazeni (pocet frame).
                   Pote nasleduje text zakoncen znakem 0;

     MGIF_COPY   - Za timto CHUNKEM je ulozen blok dat v nezakomprimovanem
                   tvaru(tj 57600byte);



  */


#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <bgraph.h>
#include <memman.h>
#include <mem.h>
#include <zvuk.h>
#include <bios.h>
//#include <vesa.h>
//#include <i86.h>
#include <io.h>
#include <mgifmem.h>
#include <strlite.h>
//#include <sys\types.h>
//#include <sys\stat.h>
#include <fcntl.h>
#include "lzwc.h"


static  char konec=0;
static  char sound_enabled=0;
static  char **titles=NULL;
static int titlesize;
static word *scrbuff=NULL;
static char redraw=0;
static char bankmode=0;
static char colr64=0;

static void *bufpos;
long vals_save=0x80008000;

static void *f,*temp;
static int posyb,posyl;
static int posy2b,posy2l;
static int posyc;
static char full_mode=0;

static char *load_buffer;
static int buf_size;
static int buf_pos;

static char screen_mode=SMD_256;

/*typedef struct mgif_header
    {
    char sign[4];
    char year[2];
    char eof;
    word ver;
    long frames;
    word snd_chans;
    int snd_freq;
    short ampl_table[256];
    short reserved[32];
    };
*/
static struct mgif_header gh;

static int mgf;

static int chunks;
static word hipal[256];

static void bread(void *what,int size)
  {
  register remain=buf_size-buf_pos;
  register readed;
  do
     {
     readed=size;
     if (readed>remain) readed=remain;
     if (readed)memcpy(what,buf_pos+load_buffer,readed);
     size-=readed;
     remain-=readed;
     what=(char *)what+readed;
     buf_pos+=readed;
     if (!remain)
        {
        read(mgf,load_buffer,buf_size);
        buf_pos=0;
        remain=buf_size;
        }
     }
  while (size);
  }


static char open_mgf_file(char *filename)
  {
  mgf=open(filename,O_BINARY | O_RDONLY);
  if (mgf==-1) return 1;
  bread(&gh,sizeof(gh));
  vals_save=0;
  bufpos=NULL;
  return 0;
  }

static void close_mgf_file()
  {
  close(mgf);
  }

static void load_frame(void *f)
  {
  long frame_head;
  long frame_size;

  bread(&frame_head,4);
  chunks=frame_head & 0xff;
  frame_size=frame_head>>8;
  bread(f,frame_size);
  }


static void load_lzw_frame(void *p,void *temp)
  {
  reinit_lzw();
  lzw_decode(p,temp);
  }


void show_full_interl_lfb(void *source,void *target,void *palette);
//#pragma aux show_full_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_lfb(void *source,void *target,void *palette);
//#pragma aux show_delta_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]
void show_full_interl_bank(void *source,void *target,void *palette);
//#pragma aux show_full_interl_bank parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_bank(void *source,void *target,void *palette);
//#pragma aux show_delta_interl_bank parm [esi][edi][ebx] modify [eax ecx edx]
void *sound_decompress(void *source,void *bufpos,int size,void *ampl_tab);
//#pragma aux sound_decompress parm [esi][edi][ecx][ebx] modify [eax edx] value [edi]
char test_next_frame(void *bufpos,int size);
//#pragma aux test_next_frame parm [edi][ecx] modify [ebx] value [al]

void show_full_interl_lfb_256(void *source,void *target,void *palette);
//#pragma aux show_full_interl_lfb_256 parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_lfb_256(void *source,void *target,void *palette);
//#pragma aux show_delta_interl_lfb_256 parm [esi][edi][ebx] modify [eax ecx edx]

static void conv_palette_256(word *pal)
  {
  register i;

  for(i=0;i<256;i++,pal++)
    {
    *pal=*((word *)xlatmem+(*pal & 0x7fff));
    }
  }

#define SWAP(a,b) a^=b,b^=a,a^=b

static void show_title(int frame)
  {
  int y,yt,h;
  static int lasty;
  char *c;

  if (frame==0) lasty=478;
  if (titlesize<=frame) return;
  if (titles[frame]==NULL) return;
  h=text_height(titles[frame])+2;
  yt=y=479-2*h;
  curcolor=0;
  bar(0,lasty,639,479);
  c=strchr(titles[frame],'\n');
  if (c!=NULL) *c=0;
  set_aligned_position(320,y,1,0,titles[frame]);
  outtext(titles[frame]);
  if (c!=NULL)
    {
    *c='\n';c++;y+=h;
    set_aligned_position(320,y,1,0,c);
    outtext(c);
    }
  if (lasty<yt) y=lasty;else y=yt;
  showview(0,y,640,480-y);
  lasty=yt;
  }

static char waited=0;

static void show_frame(void *fr,void *temp)
  {
  int x;
  char *p,a;
  long siz;
  int frmode=0,i;
  void *sound=NULL;int ssize;
  static last_counter=-1;
  int minspeed;
  p=fr;

  for(x=0;x<chunks;x++)
     {
     a=*p;siz=*(long *)p>>8;p+=4;
     switch(a)
        {
        case MGIF_LZW:
        case MGIF_DELTA:load_lzw_frame(p,temp);frmode=a;break;
        case MGIF_PAL:memcpy(hipal,p,siz);break;
        case MGIF_COPY:temp=p;frmode=a;break;
        case MGIF_SOUND:sound=p;ssize=siz;break;
        }
     p+=siz;
     }
  if (sound!=NULL)
     {
     waited=0;
     if (sound_enabled)
        {
        while (test_next_frame(bufpos,ssize)) waited=1;
        bufpos=sound_decompress(sound,bufpos,ssize,&gh.ampl_table);
        }
     else
        {
        if (last_counter==-1) last_counter=get_timer_value();
        minspeed=(ssize*50+22050)/44100;
        while (get_timer_value()-last_counter<minspeed) waited=1;;
        last_counter=get_timer_value();
        }
     }
  else Sleep(50);
  for(i=0;i<=full_mode;i++)
     {
        switch (frmode)
           {
           case MGIF_LZW:
           case MGIF_COPY:show_full_interl_lfb(temp,scrbuff+posyl,hipal);break;
           case MGIF_DELTA:show_delta_interl_lfb(temp,scrbuff+posyl,hipal);break;
           }
     }
  if (redraw && waited) showview(0,posyc,640,360);
  }

//extern int test_counter;


static void play_frames()
  {
  int x;
  for(x=0;x<gh.frames;x++)
     {
     load_frame(f);
     show_frame(f,temp);
     if (titles!=NULL) show_title(x);
     if (_bios_keybrd(_KEYBRD_READY))
        {
        konec=1;
        break;
        }
     }
  }


static void init_mgif_player(int posy)
  {
  fade_music();
  f=getmem(65536);
  temp=getmem(65536);
  posyb=scr_linelen*posy;posy2b=posyb+1280;
  posyl=scr_linelen2*posy;posy2l=posyl+scr_linelen2;
  curcolor=0;
  bar(0,posy,639,posy+360);showview(0,posy,640,360);
  posyc=posy;
  set_backsnd_freq(22050);
  }

static void done_mgif_player()
  {
  free(f);
  free(temp);
  mix_back_sound(3);
  change_music("?");
  }

static void init_load_buffer(int size)
  {
  load_buffer=getmem(buf_size=size);
  buf_pos=buf_size;
  }

static void done_load_buffer()
  {
  free(load_buffer);
  }

void play_animation(char *filename,char mode,int posy,char sound)
  {
  word *lbuffer=GetScreenAdr();
  if (scrbuff==NULL) scrbuff=lbuffer;
  screen_mode=mode & 0x7f;
  full_mode=mode>>7;
  sound_enabled=sound;
  init_load_buffer(64*1024);
  init_mgif_player(posy);
  init_lzw_compressor(8);
  if (!open_mgf_file(filename))
     {
     play_frames();
     close_mgf_file();
     }
  done_lzw_compressor();
  done_mgif_player();
  done_load_buffer();
  }

void set_title_list(char **title_list)
  {
  titles=title_list;
  if (titles!=NULL)titlesize=str_count(titles);else titlesize=0;
  }

void set_play_attribs(void *screen,char rdraw,char bm,char col64)
  {
  scrbuff=screen;
  redraw=rdraw;
  bankmode=bm;
  colr64=col64;
  }
