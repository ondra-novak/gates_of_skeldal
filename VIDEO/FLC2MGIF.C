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
#include "flc.h"
#include "lzw.h"
#include <math.h>

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

#define FRAME_X 320
#define FRAME_Y 180
#define FRAME_LEN (FRAME_X*FRAME_Y)
#define DELTA_LEN (3*FRAME_LEN)
#define LZW_LEN (2*FRAME_LEN)

#define SOUND_SPEED 44100
#define PRE_SOUND ((256*1024)/2)

#define ZTRATA lowq
#define BZTRATA colorq
#define MAX_FRAME max_fr
#define MIN_FRAME min_fr

short mult_table[64];
short square_table[4096];

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

word last_frame[FRAME_Y][FRAME_X];
char frame_delta[FRAME_LEN];
char delta_data[DELTA_LEN];
word color_state[32*32+32*32+32*32];
char calc_data[FRAME_LEN];
int delta_data_size;
int frame_delta_size;
int speed=20;

int last_frame_size;
int total_frames;

int lowq=0;
int colorq=0;
int max_fr=999999;
int min_fr=0;

int hranice;
int hdiff=0;

struct mgif_header gh;

char lzw_buffer[LZW_LEN];

char pal_use[256],color_map[256];
word hipal[256],max_colors;


FILE *mgf;

long of_pos;
int of_chunks;

void open_frame()
  {
  of_chunks=0;
  of_pos=ftell(mgf);
  fwrite(&of_pos,1,4,mgf);
  }

void close_frame()
  {
  long fsize,p;

  last_frame_size=fsize=(p=ftell(mgf))-of_pos-4;
  fseek(mgf,of_pos,SEEK_SET);
  fwrite(&of_chunks,1,1,mgf);
  fwrite(&fsize,1,3,mgf);
  fseek(mgf,p,SEEK_SET);
  total_frames++;
  }


void write_chunk(char action,long size,void *data)
  {
  fwrite(&action,1,1,mgf);
  fwrite(&size,1,3,mgf);
  fwrite(data,1,size,mgf);
  of_chunks++;
  }


void conv_2_hicolor()
  {
  int i,r,g,b;
  for(i=0;i<256;i++)
     {
     r=(flc_paleta[i][0]>>3);
     g=(flc_paleta[i][1]>>3);
     b=(flc_paleta[i][2]>>3);
     hipal[i]=((r<<10)+(g<<5)+b);
     }
  }

char find_color(word c);      //najde barvu v palete a vraci byte
#pragma aux find_color parm [eax]=\
     "mov     edi,offset hipal"\
     "mov     ecx,256"\
     "repne   scasw"\
     "mov     eax,edi"\
     "sub     eax,offset hipal"\
     "shr     eax,1"\
     "dec     eax"\
     value [al] modify[edi ecx];


void init_palmap()
  {
  int i;
  memset(pal_use,0,sizeof(pal_use));  // nuluj pristupy k barvam
  for(i=0;i<256;i++) color_map[i]=find_color(hipal[i]);  //redukuj paletu;
  max_colors=256;                     // pocet barev zatim 256;
  }

char test_transp(char c,word w)
  {
  int r1,g1,b1;
  int r2,g2,b2;
  int diff;

     r1=flc_paleta[c][0]>>3;
     g1=flc_paleta[c][1]>>3;
     b1=flc_paleta[c][2]>>3;
     w&=0x7fff;
     b2=w;g2=b2>>5;r2=g2>>5;b2&=0x1f;g2&=0x1f;
     r1-=r2;g1-=g2;b1-=b2;
     diff=abs(r1)+abs(g1<<1)+abs(b1);
  return diff<=hranice;
  }

void create_delta(char *frame_buffer)
  {
  char *delta;
  char *data;
  long colors2;
  int x,y;
  word *p;
  char skip,skc,cpc;
  char c1,c2,*a,d;

  delta=delta_data;
  data=frame_delta;
  a=frame_buffer;
  for(y=0;y<180;y++)
     {
     p=&last_frame[y];
     skip=1;
     skc=0;
     cpc=0;
     for(x=0;x<160;x++)
        {
        c1=*a++;c2=*a++;
        c1=color_map[c1];
        c2=color_map[c2];
        d=test_transp(c1,p[0]) && test_transp(c2,p[1]);
        if (d!=skip)
           {
           if (skip) *delta++=skc;else *delta++=cpc;
           skip=!skip;skc=0;cpc=0;
           }
        if (!skip)
           {
           *data++=c1;
           *data++=c2;
           colors2=hipal[c1]+(hipal[c2]<<16);
           *(long *)p=colors2;
           cpc++;
           }
        else skc++;
        p+=2;
        pal_use[c1]=1;
        pal_use[c2]=1;
        }
     if (!skip) *delta++=cpc;
     if (skc==160 && *(delta-1)!=0xff && delta!=delta_data) delta[-1]++; //preskoc n radek
     else *delta++=0xc0; //oznac konec radky
     }
  delta_data_size=delta-delta_data;
  frame_delta_size=data-frame_delta;
  }


void reduce_palette() //redukuje paletu na nejmensi nutny pocet barev
  {
  int i,j;
  for(i=0,j=0;i<256;i++)
     {
     if (pal_use[i])
        {
        hipal[j]=hipal[i];
        color_map[i]=j++;
        }
     }
  max_colors=j;
  }

void filter_colors(void *block,int size,void *colormap); //prefiltruje blok dat podle color_map
#pragma aux filter_colors parm [edi][ecx][ebx]=\
     "lp1:    mov   al,[edi]"\
        "     xlatb"\
        "     stosb"\
        "     loop  lp1"\
        modify [eax];


void *join_two_blocks(void *d1,void *d2,int siz1,int siz2,long *celk)
  {
  long siz;
  char *d;
  void *x;

  siz=siz1+siz2+4;
  d=(char *)getmem(siz);
  x=d;
  memcpy(d,&siz1,4);d+=4;
  memcpy(d,d1,siz1);d+=siz1;
  memcpy(d,d2,siz2);
  *celk=siz;
  return x;
  }

void create_last_frame(void *source,void *target,void *pal,int size);
#pragma aux create_last_frame parm [esi][edi][ebx][ecx]=\
     "lp1:    lodsb"\
         "    movzx   eax,al"\
         "    mov     eax,[eax*2+ebx]"\
         "    stosw"\
         "    loop  lp1"\
         modify[eax]


void create_mgif_pal()
  {
  write_chunk(MGIF_PAL,max_colors*2,hipal);
  }


void create_mgif_lzw()
  {
  int siz;

  conv_2_hicolor();
  init_palmap();
  create_mgif_pal();
  filter_colors(frame_buffer,FRAME_LEN,color_map);
  create_last_frame(frame_buffer,last_frame,hipal,FRAME_LEN);
  init_lzw_compressor(8);
  memset(lzw_buffer,0,sizeof(lzw_buffer));
  siz=lzw_encode(frame_buffer,lzw_buffer,FRAME_LEN);
  if (siz>FRAME_LEN) write_chunk(MGIF_COPY,FRAME_LEN,frame_buffer);
  else write_chunk(MGIF_LZW,siz,lzw_buffer);
  done_lzw_compressor(8);
  }


void create_color_state()
  {
  int i;
  int r1,g1,b1;
  int r2,g2,b2;
  int diff;
  char *c;
  word *w;

  memset(color_state,0,sizeof(color_state));
  c=frame_buffer;
  w=last_frame;
  for(i=0;i<FRAME_LEN;i++)
     {
     r1=flc_paleta[*c][0]>>3;
     g1=flc_paleta[*c][1]>>3;
     b1=flc_paleta[*c][2]>>3;
     b2=*w;g2=b2>>5;r2=g2>>5;b2&=0x1f;g2&=0x1f;
     r1-=r2;g1-=g2;b1-=b2;
     diff=abs(r1)+abs(g1<<1)+abs(b1);
     color_state[diff]++;
     c++;w++;
     }
  }

void set_max_color_change(int cchange)
  {
  int i,s;

  s=0;cchange=57600-cchange;
  for(i=sizeof(color_state)/sizeof(word)-1;s<cchange && i>0;i--) s+=color_state[i];
  if (s>=cchange) i++;
  hranice=i+hdiff;if (hranice<0) hranice=0;
  }

int rozptyl(int v1,int v2)
  {
  int c1,c2,x,s;
  x=v1+v2>>1;
  c1=(x-v1);
  c2=(x-v2);
  s=c1*c1+c2*c2;
  return s;
  }

void create_low_quality()
  {
  void *snd;int l;
  char *sn,*sr,c1,c2;
  int rs,bs,gs;

  l=FRAME_LEN;
  sr=frame_buffer;
  snd=calc_data;
  sn=(char *)snd;
  l--;
  while (l--)
     {
     c1=*sr++;
     c2=*sr;
     rs=rozptyl(flc_paleta[c1][0],flc_paleta[c2][0]);
     gs=rozptyl(flc_paleta[c1][1],flc_paleta[c2][1]);
     bs=rozptyl(flc_paleta[c1][2],flc_paleta[c2][2]);
     if (rs+gs+bs<ZTRATA)
        {
        *sn++=c1;
        *sn++=c1;
        sr++;if(!l--) break;
        }
     else *sn++=c1;
     }
  create_delta(snd);
  }

void create_mgif_delta()
  {
  void *d;
  long siz;

  conv_2_hicolor();
  init_palmap();
  if (BZTRATA) create_color_state();
  set_max_color_change(BZTRATA);
  create_low_quality();
  if (delta_data_size+frame_delta_size>FRAME_LEN)
     {
     create_mgif_lzw();
     return;
     }
  if (!frame_delta_size) return;
  reduce_palette();
  filter_colors(frame_delta,frame_delta_size,color_map);
  d=join_two_blocks(delta_data,frame_delta,delta_data_size,frame_delta_size,&siz);
  init_lzw_compressor(8);
  memset(lzw_buffer,0,sizeof(lzw_buffer));
  siz=lzw_encode(d,lzw_buffer,siz);
  done_lzw_compressor();
  free(d);
  if (siz>FRAME_LEN)
     {
     create_mgif_lzw();
     return;
     }
  create_mgif_pal();
  write_chunk(MGIF_DELTA,siz,lzw_buffer);
  }

void create_sound_track(int size)
  {
  void *p;

  p=getmem(size);
  memset(p,0,size);
  write_chunk(MGIF_SOUND,size,p);
  free(p);
  }

void reserve_track()
  {
  int size;

  size=SOUND_SPEED/speed;
  size&=~1;
  create_sound_track(size);
  }

void prereserve_tracks()
  {
  int size;
  int celk;

  size=SOUND_SPEED/speed;
  size&=~1;
  celk=PRE_SOUND;
  while (celk-size>0)
     {
     open_frame();
        create_sound_track(size);
     close_frame();
     celk-=size;
     }
  if (celk)
     {
     open_frame();
       create_sound_track(celk);
     close_frame();
     }
  }

void fill_header(int frames)
  {
  memset(&gh,0,sizeof(gh));
  strncpy(gh.sign,MGIF,4);
  strncpy(gh.year,MGIF_Y,2);
  gh.eof=26;
  gh.ver=VER;
  gh.frames=frames;
  }



void show_screen()
  {
  word *c;
  int i,j;
  word *w;

  c=last_frame;
  w=screen;
  for(i=0;i<180;i++)
     {
     for(j=0;j<320;j++)
     {
       *w++=*c;
       *w++=*c;
     c++;
     }
     w+=640;
     }
  showview(0,0,640,360);
  showview(0,400,200,20);
  }


void write_frame_size(int i,int col)
  {
     char s[20];
     curcolor=0;bar(0,400,200,420);
     sprintf(s,"%d %d %d",i,col,hranice);position(0,400);outtext(s);
  }

compress_flc(char *name)
  {
  int x;

  Open_FLC (name);
  charcolors[0]=0;
  charcolors[1]=0x7fff;
	Get_first_frame ();
	Decompress_frame ();
  curcolor=0;bar(0,0,639,479);
  open_frame();
  create_mgif_lzw();
  close_frame();write_frame_size(last_frame_size,max_colors);
  show_screen();
  for (x=2; x<=h_flc.frames; x++)
     {
		Get_next_frame ();
		Decompress_frame ();
     open_frame();
        create_mgif_delta();
     close_frame();
     write_frame_size(last_frame_size,max_colors);
     if (last_frame_size>MAX_FRAME) hdiff++;
     else if (last_frame_size<MIN_FRAME && hranice>0) hdiff--;
     else if (hdiff>0) hdiff--;else if(hdiff<0) hdiff++;
     show_screen ();
     }
  Close_FLC();
  }

void create_mgf_file(char *name)
  {
  mgf=fopen(name,"wb+");
  fwrite(&gh,1,sizeof(gh),mgf);
  total_frames=0;
  }


void close_mgf_file()
  {
  fseek(mgf,0,SEEK_SET);
  fill_header(total_frames);
  fwrite(&gh,1,sizeof(gh),mgf);
  fclose(mgf);
  }


void create_mult_and_square()
  {
  int i;

  puts("Creating tables...");
  for(i=0;i<64;i++) mult_table[i]=(short)((i-32)*(i-32));
  for(i=0;i<4096;i++) square_table[i]=(short)sqrt(i);
  }

extern int sada7;


main(int argc,char *argv[])
  {
  puts("\n(C)1997 Komprimator Motion GIF v0.9 by Ondrej Novak");
  if (argc<3)
     {
     puts("Pouziti:");putchar('\n');;
     puts("FLCMGIF source.flc target.mgf");putchar('\n');
     puts("Tento program generuje soubor MGF z originalniho FLC bez vkladani\n"
          "prazdne zvukove stopy a bez ztratove kompresse (pouze convertor)");
     exit(0);
     }
  create_mgf_file(argv[2]);
  initmode32();
  curfont=(void *)&sada7;
  compress_flc(argv[1]);
  closemode();
  puts("Zaviram Motion GIF...");
  close_mgf_file();
  }
