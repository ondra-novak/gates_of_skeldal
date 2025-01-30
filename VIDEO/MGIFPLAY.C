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
#include <libs/types.h>
#include <libs/bgraph.h>
#include <libs/memman.h>
#include <mem.h>
#include "lzw.h"
#include <libs/zvuk.h>

#include <vesa.h>
#include <i86.h>
#include <io.h>
//#include <sys\types.h>
//#include <sys\stat.h>
#include <fcntl.h>

#define MGIF "MGIF"
#define MGIF_Y "97"
#define VER 0x100
#define MGIF_EMPTY  0
#define MGIF_LZW    1
#define MGIF_DELTA  2
#define MGIF_PAL    3
#define MGIF_SOUND  4
#define MGIF_TEXT   5
#define MGIF_COPY   6
#define MGIF_SINIT  7

char konec=0;

void *bufpos;
long vals_save=0x80008000;


typedef struct mgif_header
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

struct mgif_header gh;

int mgf;

int chunks;
word hipal[256];

char open_mgf_file(char *filename)
  {
  mgf=open(filename,O_BINARY | O_RDONLY);
  if (mgf==-1) return 1;
  read(mgf,&gh,sizeof(gh));
  vals_save=0;
  bufpos=NULL;
  return 0;
  }

void close_mgf_file()
  {
  close(mgf);
  }

void load_frame(void *f)
  {
  long frame_head;
  long frame_size;

  read(mgf,&frame_head,4);
  chunks=frame_head & 0xff;
  frame_size=frame_head>>8;
  read(mgf,f,frame_size);
  }


void load_lzw_frame(void *p,void *temp)
  {
  reinit_lzw();
  lzw_decode(p,temp);
  }

void display_copy(char *temp)
  {
  char *c;
  int i,j;
  word *w,ww;

  c=temp;
  w=lbuffer+60*640;
  for(i=0;i<180;i++)
     {
     for(j=0;j<320;j++)
     {
       ww=hipal[*c];
//       w[640]=ww;
       *w++=ww;
//       w[640]=ww;
       *w++=ww;
       c++;
     }
     w+=640;
     }
  //showview(0,0,640,360);
  }

void display_delta(char *temp)
  {
  char *graph,*contr,sk;
  int i,j,k;
  word *w,ww;

  contr=temp;
  graph=contr+*(long *)contr;
  contr+=4;graph+=4;
  for(i=0;i<180;i++)
     {
     w=lbuffer+1280*(i+30);sk=1;
       while ((*contr & 0xc0)!=0xc0)
        if (sk)
           {
           w+=(*contr++)*4;sk=!sk;
           }
        else
           {
           k=*contr++*2;
           for(j=0;j<k;j++)
              {
              ww=hipal[*graph];
//             w[640]=ww;
              *w++=ww;
//             w[640]=ww;
              *w++=ww;
              graph++;
              }
           sk=!sk;
           }
        i+=*contr++ & 0x3f;
     }
//  showview(0,0,640,360);
  }


void show_full_interl_lfb(void *source,void *target,void *palette);
#pragma aux show_full_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_lfb(void *source,void *target,void *palette);
#pragma aux show_delta_interl_lfb parm [esi][edi][ebx] modify [eax ecx edx]
void show_full_interl_bank(void *source,void *target,void *palette);
#pragma aux show_full_interl_bank parm [esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_bank(void *source,void *target,void *palette);
#pragma aux show_delta_interl_bank parm [esi][edi][ebx] modify [eax ecx edx]
void *sound_decompress(void *source,void *bufpos,int size,void *ampl_tab);
#pragma aux sound_decompress parm [esi][edi][ecx][ebx] modify [eax edx] value [edi]
char test_next_frame(void *bufpos,int size);
#pragma aux test_next_frame parm [edi][ecx] modify [ebx] value [al]


void show_frame(void *fr,void *temp)
  {
  int x;
  char *p,a;
  long siz;
  int frmode=0;
  void *sound=NULL;int ssize;

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
     while (test_next_frame(bufpos,ssize));
     bufpos=sound_decompress(sound,bufpos,ssize,&gh.ampl_table);
     }
  else delay(50);
  if (banking)
  switch (frmode)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_interl_bank(temp,(word *)(60*2048),hipal);break;
     case MGIF_DELTA:show_delta_interl_bank(temp,(word *)(60*2048),hipal);break;
     }
  else
  switch (frmode)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_interl_lfb(temp,lbuffer+60*640,hipal);break;
     case MGIF_DELTA:show_delta_interl_lfb(temp,lbuffer+(60)*640,hipal);break;
     }
  }

extern int test_counter;


void play_frames()
  {
  int x;
  void *f,*temp;
  f=getmem(65536);
  temp=getmem(65536);
  for(x=0;x<gh.frames;x++)
     {
     load_frame(f);
     show_frame(f,temp);
     if (_bios_keybrd(_KEYBRD_READY))
        {
        konec=1;
        break;
        }
     }
  test_counter-=gh.frames+40;
  free(f);
  free(temp);
  }


main(int argc,char *argv[])
  {
  int a,b,c,d;
  if (argc==1)
     {
     puts("Pouziti:");
     puts("MGIFPLAY filename.mgf [snd_device] [port] [dma] [irq]");
     exit(0);
     }
  initmode32();
  if (argc>2)
     {
     sscanf(argv[2],"%d",&a);
     sscanf(argv[3],"%x",&b);
     sscanf(argv[4],"%d",&c);
     sscanf(argv[5],"%d",&d);
     }
  else sound_detect(&a,&b,&c,&d);
  set_mixing_device(a,22050,b,c,d);
  start_mixing();delay(10);
  init_lzw_compressor(8);
  curcolor=0x0;
  bar(0,0,639,479);showview(0,0,0,0);
  do
     if (open_mgf_file(argv[1])) konec=1;
     else
        {
        play_frames();
        close_mgf_file();
        }
  while (!konec);
  done_lzw_compressor();
  stop_mixing();
  closemode();
  }

