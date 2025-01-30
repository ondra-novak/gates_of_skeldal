#include <stdio.h>
#include <libs/zvuk.h>
#include <conio.h>
#include <mem.h>
#include <stdlib.h>

#include <dos.h>
#include <i86.h>
#include <libs/strlite.h>
#include <libs/memman.h>

#define SCRADR 0xb8000
#define SCRSIZE (80*25*2)
#define SCRLLEN 80
#define VOL_BLOCK (bfreq/44)
#define BVSTEP 2350

extern char backsndbuff[BACK_BUFF_SIZE];
extern volatile long backsnd;
extern volatile long backstep;
extern volatile int  backfine;
extern int bfreq,bblocks,bvolume;
extern FILE *bsnd;
extern unsigned short bchans;
extern long blength;
extern int bblock_size;
extern int device;
extern int devport;

char manual_refresh=0;

int get_playing_time(char *filename)
  {
  open_backsound(filename);
  fclose(bsnd);
  if (blength!=-1) return (bblocks*bblock_size)/(bfreq*bchans);else return 0;
  }

TSTR_LIST read_dir(char *mask,int attrs)
  {
  TSTR_LIST flist;
  int index=0;
  char c[80];
  struct find_t s;
  int rc;
  int doba;

  flist=create_list(256);
  if (flist==NULL) return flist;
  rc=_dos_findfirst(mask,attrs,&s);
  while (rc==0)
     {
     char d[13],*p;
     int i=0,j;

     if (attrs==_A_NORMAL || s.attrib & attrs)
        {

        p=&d;d[12]='\0';
        while (s.name[i]!='.' && s.name[i]!='\0' ) *p++=s.name[i++];
        if (s.name[i]!='\0') j=i+1;else j=i;
        while (i<8)
          {
          *p++=32;i++;
          }
       i=3;
       *p++='.';
       while (s.name[j]!='\0')
          {
          *p++=s.name[j++];
          i--;
          }
       while (i>0)
          {
          *p++=32;
          i--;
          }
       doba=get_playing_time(s.name);
       sprintf(c,"%s %02d:%02d",d,doba/60,doba%60);
     if (str_replace(&flist,index++,c)==NULL)
        {
        release_list(flist);
        return NULL;
        }
        }
     rc=_dos_findnext(&s);
     }
  sort_list(flist,-1);
  str_delfreelines(&flist);
  return flist;
  }

void name_conv(char *c)
  {
  char *a,*b,cn,cd;

  a=c;b=c;
  cn=4;cd=0;
  for(;*a && cn;a++,cn-=cd)
     {
     if (*a!=32) *b++=*a;
     if (*a=='.') cd=1;
     }
  *b--='\0';
  if (*b=='.') *b='\0';
  }



long music_size;
int block_size;
int block_max;
int vybrano=0;
int posledni=-1;
char error=0;

typedef char TDIGIT[10];

TSTR_LIST adresar;

char istr[300];
char pause=0;

unsigned short *screen;

TDIGIT digits[15]=
  {   "���"
      "� �"
      "���",
      "  �"
      "  �"
      "  �",
      "���"
      "���"
      "���",
      "���"
      "���"
      "���",
      "� �"
      "���"
      "  �",
      "���"
      "���"
      "���",
      "���"
      "���"
      "���",
      "���"
      "� �"
      "  �",
      "���"
      "���"
      "���",
      "���"
      "���"
      "���",
      " � "
      " � "
      "   ",
      "   "
      "���"
      "   "
  };


void load_font(char *font,int start,int count);
#pragma aux load_font =\
           "cli"\
           "mov   edx,3c4h"\
           "mov   eax,0402h"\
           "out   dx,ax"\
           "mov   eax,0704h"\
           "out   dx,ax"\
           "Mov   edx,3ceh"\
           "mov   eax,0204h"\
           "out   dx,ax"\
           "mov   eax,0005h"\
           "out   dx,ax"\
           "mov   ax,0406h"\
           "out   dx,ax"\
           "shl   ecx,5"\
           "mov   edx,edi"\
           "mov   edi,0a0000h"\
           "add   edi,ecx"\
   "Opk:    mov   ecx,4"\
           "rep   movsd"\
           "add   edi,16"\
           "dec   edx"\
           "jne   opk"\
           "mov   edx,3c4h"\
           "mov   eax,0302h"\
           "out   dx,ax"\
           "mov   eax,0304h"\
           "out   dx,ax"\
           "mov   edx,3ceh"\
           "mov   eax,0004h"\
           "out   dx,ax"\
           "mov   eax,1005h"\
           "out   dx,ax"\
           "mov   eax,0E06h"\
           "out   dx,ax"\
           "sti"\
        parm [ESI][ECX][EDI] modify [EDX EAX];

void font_effekt1();
#pragma aux font_effekt1 =\
           "mov   ecx,32"\
           "mov   edi,127"\
           "mov   edx,3c4h"\
           "mov   eax,0402h"\
           "out   dx,ax"\
           "mov   eax,0704h"\
           "out   dx,ax"\
           "Mov   edx,3ceh"\
           "mov   eax,0204h"\
           "out   dx,ax"\
           "mov   eax,0005h"\
           "out   dx,ax"\
           "mov   ax,0406h"\
           "out   dx,ax"\
           "shl   ecx,5"\
           "mov   edx,edi"\
           "mov   esi,0a0000h"\
           "mov   edi,0a4000h"\
           "push  ecx"\
           "mov   ecx,2000h"\
           "rep   movsb"\
           "pop   ecx"\
           "mov   edi,0a4000h"\
           "add   edi,ecx"\
         modify [EDX EAX ESI ECX EDI];

void font_effekt2();
#pragma aux font_effekt2 =\
   "Opk:    mov   al,0ffh"\
           "stosb"\
           "add   edi,14"\
           "stosb"\
           "add   edi,16"\
           "dec   edx"\
           "jne   opk"\
           "mov   edx,3c4h"\
           "mov   eax,0302h"\
           "out   dx,ax"\
           "mov   eax,0304h"\
           "out   dx,ax"\
           "mov   edx,3ceh"\
           "mov   eax,0004h"\
           "out   dx,ax"\
           "mov   eax,1005h"\
           "out   dx,ax"\
           "mov   eax,0E06h"\
           "out   dx,ax"\
         modify [EDX EAX ESI ECX EDI];
void font_effekt()
  {
  font_effekt1();
  font_effekt2();
  }

#pragma aux dve_sady modify [EAX EBX EDX]=\
           "mov   ebx,1"\
           "mov   eax,1103h"\
           "int   10h"\
           "cli"\
           "mov   edx,3d4h"\
           "mov   eax,0100h"\
           "out   dx,ax"\
           "mov   edx,3c4h"\
           "mov   al,1"\
           "out   dx,al"\
           "inc   edx "\
           "in al,dx "\
           "or al,1"\
           "out   dx,al"\
           "mov  edx,03dah"\
           "in   al,dx"\
           "mov  edx,03c0h"\
           "mov  al,13h"\
           "out  dx,al"\
           "mov  al,0"\
           "out   dx,ax"\
           "mov  al,32"\
           "out  dx,al"\
           "mov   edx,3d4h"\
           "mov   eax,0300h"\
           "out   dx,ax"\
           "sti"\

void dve_sady();

void set_palcolor(int index,int r,int g,int b);
#pragma aux set_palcolor parm [ebx][edx][eax][ecx]=\
  "mov  ch,al"\
  "mov  dh,dl"\
  "mov  eax,1010h"\
  "int  10h"


void def_fbars()
  {
  char f_bars[8][16];
  int i,j;

  memset(f_bars,0,sizeof(f_bars));
  for(i=0;i<8;i++)
     for(j=0;j<=i;j++)
       f_bars[7-i][15-j*2]=255;
  load_font(f_bars,192,8);
  for(i=0;i<16;i++) f_bars[0][i]=1;
  for(i=0;i<16;i++) f_bars[1][i]=192;
  for(i=0;i<16;i++) f_bars[2][i]=255*(i>13);
  load_font(f_bars,200,3);
  }


void disp_digit(int x,int y,char num)
  {
  unsigned short *adr;
  char *c;

  adr=screen+y*SCRLLEN+x;
  c=&digits[num];
  for(y=0;y<3;y++,adr+=SCRLLEN-3)
     for(x=0;x<3;x++) *(char *)(adr++)=*c++;
  }

void clear_window(int x1,int y1,int x2,int y2,int color)
  {
  int x,y;
  unsigned short *adr;

  color<<=8;color+=32;
  for(y=y1;y<=y2;y++)
     {
     adr=screen+y*SCRLLEN;
     for(x=x1;x<=x2;x++)
        adr[x]=color;
     }
  }

void disp_str(int x,int y,char *c,int color)
  {
  unsigned short *adr;

  adr=screen+y*SCRLLEN+x;
  while (*c) *adr++=*(c++)+(color<<8);
  }

void disp_vbar_track(int x,int y,int step)
  {
  unsigned short *adr;
  int drw=0;char cl;

  adr=screen+y*SCRLLEN+x;
  while (drw<32768)
     {
     cl=15;
     drw+=step;
     adr[0]=200+(cl<<8);
     adr[3]=201+(cl<<8);
     adr-=SCRLLEN;
     }
  }

void disp_bnum(int x,int y,int num,int dgs)
  {
  char sgn;

  sgn=num<0;num=abs(num);
  while (dgs>0 || num>0)
     {
     disp_digit(x,y,num%10);
     num/=10;dgs--;
     x-=4;
     }
  if (sgn) disp_digit(x,y,11);
  }

void disp_chan(int x,int y,int value,int step)
  {
  unsigned short *adr;
  int drw=0;char cl;

  adr=screen+y*SCRLLEN+x;
  while (drw<32768)
     {
     if (drw>=value) cl=0;
     else if (drw>24660) cl=12;
     else if (drw>16384) cl=14;
     else cl=10;
     drw+=step;
     if (drw>value && cl>0)
        *adr=192+8*(drw-value)/step+(cl<<8);
     else
        *adr=192+(cl<<8);
     adr-=SCRLLEN;
     }
  *adr=202+(15<<8);
  }

void init(int a,int b,int c,int d)
  {
  screen=(unsigned short *)SCRADR;
  clear_window(0,0,79,24,15);
  disp_str(0,0,"Output Device:",9);
  disp_str(0,1,"Parameters:",9);
  disp_str(0,3,"Track name:",9);
  disp_str(0,4,"Mode:",9);
  disp_str(0,5,"Blocks:",9);
  disp_str(0,6,"Size:",9);
  disp_str(0,7,"Playing time:",9);
  if (a==-1)
  if (sound_detect(&a,&b,&c,&d))
     {
     disp_str(16,0,"No sound device found",14);
     exit(1);
     }
  set_mixing_device(a,22050,b,c,d);
  disp_str(16,0,device_name(a),14);
  if (a>=DEV_SB10 && a<=DEV_ULTRA)
     {
     sprintf(istr,"port %03X dma %1d irq %1X",b,c,d);
     disp_str(16,1,istr,13);
     }
  clear_window(50,0,79,3,30);
  clear_window(50,4,79,7,4*16+15);
  }

void open_files(char *filename,char fade)
  {
  long total;
  char spaces[50];

  memset(spaces,32,50);spaces[30]=0;
  if (bsnd!=NULL)fclose(bsnd);
  if (filename!=NULL)if (fade) change_music(filename);else open_backsound(filename);
  else blength=-1;
  music_size=blength;
  block_size=bblock_size;
  disp_str(16,3,spaces,12);
  error=0;
  if (music_size==-1)
        {
        disp_str(16,3,"Track NOT found",12);
        block_size=1;
        bchans=1;
        bfreq=1;
        bblocks=0;
        error=1;
        }
  else  disp_str(16,3,filename,12);
  if (bchans==1)
  sprintf(istr,"16-bit mono %5d Hz  ",bfreq);
  else
  sprintf(istr,"16-bit stereo %5d Hz",bfreq);
  disp_str(16,4,istr,11);
  sprintf(istr,"%05d",bblocks);
  disp_str(16,5,istr,10);
  sprintf(istr,"%05d KB",music_size/1024);
  disp_str(16,6,istr,15);
  total=(bblocks*block_size)/(bfreq*bchans);
  sprintf(istr,"%02d:%02d",total/60,total%60);
  disp_str(16,7,istr,14);
  block_max=bblocks;
  }

void display_time(int bblocks)
  {
  long time,b;

  if (error)
     {
     disp_bnum(76,1,88,2);
     disp_digit(68,1,10);
     disp_bnum(64,1,88,2);
     }
  else
     {
     b=bblocks+128*1024/block_size;
     if (b>block_max) b-=block_max;
     time=(b*block_size)/(bfreq*bchans);
     disp_bnum(76,1,time%60,2);
     disp_digit(68,1,10);
     disp_bnum(64,1,time/60,2);
     }
  }

void display_bblocks(int bblocks)
  {
  long b;

  if (error)
     disp_bnum(76,5,88888,5);
  else
     {
     b=bblocks+128*1024/block_size;
     if (b>block_max) b-=block_max;
     disp_bnum(76,5,b,5);
     }
  }

void volume_bars(int *left,int *right)
  {
  int i,k,l,r;
  static last=0;
  static sl,sr;

  r=backsnd-last;
  if (r<0) r+=BACK_BUFF_SIZE/4;
  if (r>VOL_BLOCK)
     {
     *left=sl;
     *right=sr;
     sr-=1000;
     sl-=1000;
     last=backsnd;
   for(i=0;i<VOL_BLOCK;i++)
     {
     k=backsnd+i;
     if (k>=BACK_BUFF_SIZE/4) k-=BACK_BUFF_SIZE/4;;
     l=*((short *)backsndbuff+k*2);
     r=*((short *)backsndbuff+k*2+1);
     if (l>sl)
        sl=l;
     if (r>sr)
        sr=r;
     }
     }
  }
char retrace()
//vraci stav kresliciho paprsku na obrazovce
  {
  return (((~inp(0x3da)) & 8)>>3);
  }

void wait_retrace()
//ceka na zpetny chod
  {
  while (!retrace());
  while (retrace());
  }

void disp_volume()
  {
  disp_chan(77,23,bvolume*64,BVSTEP);
  disp_chan(78,23,bvolume*64,BVSTEP);
  disp_vbar_track(76,23,BVSTEP);
  }

void show_dir(int sel_file,int playing)
  {
  int cn=str_count(adresar);
  int i=sel_file-5;
  char spaces[32];
  int color;

  memset(spaces,32,32);
  spaces[31]=0;
  for(;i<sel_file+5;i++)
     {
     if (sel_file==i) color=3+(playing==i?2:0);
     else color=9+(playing==i?2:0);
     if (i<0 || i>=cn || adresar[i]==NULL)disp_str(3,i-sel_file+17,spaces,0);
     else disp_str(3,i-sel_file+17,adresar[i],color);
     }
  color=3+(playing==sel_file?2:0);
  disp_str(1,17,"  ",color);
  disp_str(21,17,"  ",color);
  disp_str(0,17,"�",color);
  disp_str(23,17,"�",color);
  }
void play_next(char **next)
  {
  posledni++;
  if (posledni>=str_count(adresar)) posledni=0;
  if (adresar[posledni]==NULL) posledni=0;
  if (adresar[posledni]==NULL) {*next=NULL;posledni=0;}
  else
     {
     static char c[50];
     vybrano=posledni;
     show_dir(vybrano,posledni);
     strcpy(c,adresar[vybrano]);
     name_conv(c);
     open_files(c,0);
     fclose(bsnd);
     *next=c;
     }
  }

void set_new_position()
  {
  static int l=0,r=0;
  int i=bblocks;
  long c;
  int delta1=5,delta2=1;
  int volsave;
  char zn,oldzn;

  c=get_timer_value();
  do
     {
     if (_bios_keybrd(_KEYBRD_READY))
       switch (zn=_bios_keybrd(_KEYBRD_READ)>>8)
          {
          case 'M':i-=delta2;
                  if (i<0) i=0;
                  display_time(i);
                  c=get_timer_value();
                  delta1--;
                  break;
          case 'K':i+=delta2;if (i>=block_max) i=block_max-1;
                  display_time(i);
                  c=get_timer_value();
                  delta1--;
                  break;
          }
 if (delta1<0)
     {
     delta1=5;
     delta2++;
     }
  if (zn!=oldzn) delta2=1;
  oldzn=zn;
  display_bblocks(bblocks);
  volume_bars(&l,&r);
  mix_back_sound(0);
  disp_chan(37,23,l,BVSTEP);
  disp_chan(38,23,l,BVSTEP);
  disp_chan(41,23,r,BVSTEP);
  disp_chan(42,23,r,BVSTEP);
     }
  while (get_timer_value()-c<50);
  if (i==bblocks) return;
  volsave=get_snd_effect(SND_MUSIC);
  set_snd_effect(SND_MUSIC,0);
  set_snd_effect(SND_MUSIC,volsave);;
  delta1=0;
  if (i>bblocks && (i-bblocks)<(block_max-i))
     {
     fseek(bsnd,4,SEEK_CUR);
     for(;bblocks<i;bblocks++)
        {
        fseek(bsnd,-(12+block_size),SEEK_CUR);
        fread(&delta1,1,4,bsnd);
        if (delta1!=block_size) break;
        display_bblocks(bblocks);
        }
     fseek(bsnd,-4,SEEK_CUR);
     }
  if (i>bblocks) open_backsound(NULL);
  for(;bblocks!=i;bblocks--)
     {
     fread(&delta1,1,4,bsnd);
     fseek(bsnd,4+delta1,SEEK_CUR);
     display_bblocks(bblocks);
     }
  }

extern int bxbass;
extern unsigned short predstih;
extern int dmatable[8][4];
extern char intr_test;


static void print_dma(int dma_num)
  {
  int i,j,k;

  i=inp(dmatable[dma_num][0]);
  j=inp(dmatable[dma_num][0]);
  k=inp(dmatable[dma_num][2]);
  cprintf("%04X %0X|",i+j*256,k);
  }

static void print_all_dma()
  {
  int i;
  static char negate;

  for(i=0;i<8;i++) print_dma(i);
  if (intr_test)
     {
     negate=!negate;
     intr_test=0;
     }
  cprintf(" %c %d\r",negate?'*':' ',predstih);
  }

void MIXER();
#pragma aux MIXER modify [eax ebx ecx edx esi edi];


char refresh()
  {
  static int l=0,r=0;
  char *c;
  int z;
  mix_back_sound(0);
  if (manual_refresh)
     {
     MIXER();
     predstih=0x8000;
     if (device==DEV_SB16)
        {
        int i;
          i=inp(devport+0xf);
          i=inp(devport+0xe);
        }
     }
  else
     {
     display_bblocks(bblocks);
     display_time(bblocks);
     print_all_dma();
     volume_bars(&l,&r);
     disp_chan(37,23,l,BVSTEP);
     disp_chan(38,23,l,BVSTEP);
     disp_chan(41,23,r,BVSTEP);
     disp_chan(42,23,r,BVSTEP);
     }
  if (error && adresar[0]!=NULL)
     {
     play_next(&c);
     open_files(c,0);
     }
  while (_bios_keybrd(_KEYBRD_READY))
     switch (_bios_keybrd(_KEYBRD_READ)>>8)
     {
     case 1:return 0;
     case 'M':
     case 'K': set_new_position();break;
     case 0x39:if (pause) start_mixing(); else stop_mixing();
               pause=!pause;
               break;
     case 0x17:set_snd_effect(SND_OUTFILTER,2);break;
     case 'I': if (bvolume<512) bvolume+=4;
                 disp_volume();
                 break;
     case 'Q': if (bvolume>3) bvolume-=4;else bvolume=0;
                 disp_volume();
                 break;
     case 'G': if (check_snd_effect(SND_LSWAP))
                 {
                 z=get_snd_effect(SND_LSWAP);
                 z+=0xf;if (z>0xff) z=0xff;
                 set_snd_effect(SND_LSWAP,z);
                 }break;
     case 'O': if (check_snd_effect(SND_LSWAP))
                 {
                 z=get_snd_effect(SND_LSWAP);
                 z-=0xf;if (z<0) z=0;
                 set_snd_effect(SND_LSWAP,z);
                 }
     case 'H': if (vybrano>0)
                 {
                 vybrano--;show_dir(vybrano,posledni);
                 }break;
               break;
     case 'P': if (vybrano+1<str_count(adresar) && adresar[vybrano+1]!=NULL)
                 {
                 vybrano++;show_dir(vybrano,posledni);
                 }
               break;
     case ';':bxbass+=16;if (bxbass>256) bxbass=256;break;
     case '<':bxbass-=16;if (bxbass<0) bxbass=0;break;
     case 0x1c:if (adresar[vybrano]!=NULL)
               {
               char c[50];
               posledni=vybrano;
               show_dir(vybrano,posledni);
               strcpy(c,adresar[vybrano]);
               name_conv(c);
               open_files(c,vybrano);
               break;
               }

     }
  return 1;
  }


void restore_mode();
#pragma aux restore_mode=\
        "mov  eax,3"\
        "int 10h"\
        modify [eax];

main(int argc,char *argv[])
  {
  int a,b,c,d;

  adresar=read_dir("*.mus",_A_NORMAL);
  if (argc!=2 && argc!=6 && (argc!=1 || adresar[0]==NULL))
     {
     int i;
     printf("\n\nUsage:  TRACK trackname [device] [port] [dma] [irq] \n"
              "  or      TRACK * [device] [port] [dma] [irq] - plays current directory."
              "\n\nDevice numbers:\n");
     for (i=0;i<7;i++) printf("%d. %s\n",i,device_name(i));
     puts("\nIf you use character '~' (tilda) as trackname, it will have the same\n"
          "effect as '*' so you will able to play sound background in WIN95\n"
          "But the volume bars will not display correct values.");
     exit(0);
     }
  if (argc==6)
     {
     sscanf(argv[2],"%d",&a);
     sscanf(argv[3],"%x",&b);
     sscanf(argv[4],"%d",&c);
     sscanf(argv[5],"%d",&d);
     }
  else
     a=-1;
  if (argv[1][0]=='~') manual_refresh=1;
  def_fbars();
  //dve_sady();
  //font_effekt();
  set_palcolor(57,0,42,42);
  set_palcolor(5,0,63,63);
  init(a,b,c,d);
  open_files(argv[1],0);
  start_mixing();
  disp_vbar_track(36,23,BVSTEP);
  disp_vbar_track(40,23,BVSTEP);
  disp_volume();
  show_dir(0,1);
  konec_skladby=play_next;
  while (refresh());
  stop_mixing();
  clear_window(0,0,79,24,15);
  restore_mode();
  }
