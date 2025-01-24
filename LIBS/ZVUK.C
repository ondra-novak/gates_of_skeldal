#include <stdlib.h>
#include <stdio.h>
//#include <i86.h>
//#include <dpmi.h>
#include <conio.h>
#include <mem.h>
#include <bios.h>
#include <dos.h>
#include "zvuk.h"
//#include "pcspeak.h"

#define MIX_REZERVA 0x50
#define XBASS_PARM 5
/*
#define build_dac_xlat() \
                       {\
                          int i;\
                          for(i=0;i<256;i++) da_xlat[i]=i;\
                       }\

#define sendsb(data) \
                 *countdown=10;\
                 while ((inp(devport+0xc) & 0x80) && *countdown);\
                 outp(devport+0xc,data)

#define recivesb(data)\
                 *countdown=10;\
                 while (!(inp(devport+0xe) & 0x80) && *countdown);\
                 data=inp(devport+0xa)

#define sendwss(reg,data)\
                 *countdown=10;\
                 while ((inp(devport+0x4) & 0x80) && *countdown);\
                 outp(devport+0x4,reg);\
                 outp(devport+0x5,data);

#define recivewss(reg,data)\
                 *countdown=10;\
                 while ((inp(devport+0x4) & 0x80) && *countdown);\
                 outp(devport+0x4,reg);\
                 data=inp(devport+0x5);



char detect_enables[8];
#define irqdetect(irq) \
                 {\
                 if (detect_enables[irq]) \
                    devirq=irq;\
                 detect_enables[irq]=0;\
                 inp(devport+0xE);\
                 outp(0x20,0x20);\
                 }

*/
typedef struct tchannel
  {
  char *play_pos, *start_loop, *end_loop;
  long speed_maj;
  unsigned short speed_min, minor_pos, sample_type, vol_left, vol_right;
  }TCHANNEL;

TCHANNEL chaninfo[32];
char *mixbuffer=NULL;
char backsndbuff[BACK_BUFF_SIZE];
volatile long backsnd=0;
volatile long backstep=0x10000;
volatile int  backfine=0;
long ticker_save;
/*long jumptable[3];
long getdma;
long ido;*/
unsigned short predstih=0x960;
long lastdma;
long lastmix;
long mixpos;
long surpos;
long mixsize;
long dmaposadr;
long dmasizadr;
long dmapageadr;
long dmanum;
int dmamask;
int dmamode;
int dmamodenum=0x58;//rezim DMA (default pro SB - POZOR zmenit pro GUS na 0x48
int device;
long samplerate=22050;
long mixfreq=50;
long idt_map[2];
int call_back_data;
int call_back_sel;
static char *countdown=(char *)0x440;
unsigned short dpmiselector;
char mixer_zavora=0;
static char hw_intpol=0;
FILE *bsnd;
unsigned short bchans;
short btable[256];
int bfreq,bblocks,bblock_size,bvolume=255;
int bxbass=0;
static int gfxvol=255;
long blength;
static char depack[32768];
void (__far __interrupt *oldvect)();
static char swap_chans=0;

char da_xlat[256]; // Xlat tabulka pro DA a PC Speaker;


int devport=0x220; //device base port
int devirq=0;      //device main irq

char gus_dma_type=0x41;  //typ dma prenosu pro GUS (default 8-bit dma 16-bit samples highspeed)
char gus_last_buff=0;


int test_counter=0;
int timer_test_port;
int timer_value=0;
char intr_test;

void (*konec_skladby)(char **jmeno)=NULL;

int dmatable[8][4]=
  {
  {0,1,0x87,0x0A0B},
  {2,3,0x83,0x0A0B},
  {4,5,0x81,0x0A0B},
  {6,7,0x82,0x0A0B},
  {0xc0,0xc2,0x8f,0xD4D6},
  {0xc4,0xc6,0x8b,0xD4D6},
  {0xc8,0xca,0x89,0xD4D6},
  {0xcc,0xce,0x8a,0xD4D6}
  };

static int irqtable[16]={8,9,0xa,0xb,0xc,0xd,0xe,0xf,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77};

  static char names[][25]=
     {
     "Unsupported device",
     "Nosound",
     "Sound Blaster",
     "Sound Blaster 2",
     "Sound Blaster Pro",
     "Sound Blaster 16+",
     "Windows Sound System",
     "Gravis Ultrasound",
     "DAC on LPT",
     "Internal Hooker",
     "Pro Audio Spectrum"
     };

char *buff_alloc();
#pragma aux buff_alloc modify[ebx ecx edx] value [eax]
void mixer();
#pragma aux mixer modify [eax ebx ecx edx esi edi]
void setsbpro();
#pragma aux setsbpro modify [eax edi]
void setsb2();
#pragma aux setsb2 modify [eax edi]
void setsb2_s();
#pragma aux setsb2_s modify [eax edi]
void setsb16();
#pragma aux setsb16 modify [eax edi]
void setgus();
#pragma aux setgus modify [eax edi]
void init_gus();
#pragma aux init_gus modify [eax ecx edx edi]
void stop_gus();
#pragma aux stop_gus modify [eax ecx edx edi]
void gus_setchan_vol(int voice,int volume);
#pragma aux gus_setchan_vol parm [eax][ecx] modify [edx]
void gus_setchan_pan(int voice,int volume);
#pragma aux gus_setchan_pan parm [eax][ecx] modify [edx]


void init_dma(int dma)
{
dmanum=dma;
dmaposadr=dmatable[dma][0];
dmasizadr=dmatable[dma][1];
dmapageadr=dmatable[dma][2];
dmamask=dmatable[dma][3] >> 8;
dmamode=dmatable[dma][3] & 0xff;
}

void prepare_dma(char *adr,int block)
{
int poz;int page;

outp(0x0C,0);outp(0xD8,0);
outp(dmamask,(dmanum & 3)+0x04);
outp(dmamode,(dmanum & 3)+dmamodenum);
if (dmanum>3)
  {
  block>>=1;
  poz=(long)adr>>1;
  page=((long)adr>>16) & ~1;
  }
else
  {
  poz=(long)adr;
  page=(long)adr>>16;
  }
block--;
outp(dmaposadr,poz & 0xff);
outp(dmaposadr,(poz>>8) & 0xff);
outp(dmapageadr,page);
outp(dmasizadr,block & 0xff);
outp(dmasizadr,block>>8);
outp(dmamask,dmanum & 3);
/*
printf("adr: %p\n"
       "block: %04X\n"
       "pos: %04X\n"
       "map: %04X\n"
       "dmapage: %X\n"
       "dmapos: %X\n"
       "dmasize: %X\n"
       "dmamask: %X\n",
       adr,block,poz & 0xffff,page,dmapageadr,dmaposadr,dmasizadr,dmamask);
getche();
 */
}



void reset_blaster()
{
int counter=100;
outp(devport+0x6,1);
delay(10);
outp(devport+0x6,0);
while (inp(devport+0x0A)!=0xAA && counter--) delay(1);
}
void start_sbmono()
{
prepare_dma(mixbuffer,65536);
sendsb(0xd1);
sendsb(0x40);
sendsb(256 - (1000000 / samplerate));
sendsb(0x48);
sendsb(0xff);
sendsb(0xff);
sendsb(0x1c);
}

void stop_sbmono()
{
sendsb(0xd3);
sendsb(0xd0);
//sendsb(0xda);
//sendsb(0xd0);
}

void start_sb16();

void __interrupt __far sb16_irq()
  {
  int i=32767;

  i=inp(devport+0xf);
  i=inp(devport+0xe);
//  sendsb(0xb6);
//  sendsb(0x30);
//  sendsb(i & 0xff);
//  sendsb(i >> 8);  //play(16bit,signed,stereo,16384);
  outp(0x20,0x20);
  outp(0xA0,0x20);
  intr_test=1;
  }
void start_sb16()
  {
  int i=16383;

  outp(devport+0x4,0x81);
  if (dmanum>3)
     outp(devport+0x5,(1 << dmanum)| (inp(devport+0x5) & 0xf));
  else
     outp(devport+0x5,1 << dmanum);
  outp(devport+0x4,0x80);
  switch (devirq)
     {
     case 2:outp(devport+0x5,0xf1);break;
     case 5:outp(devport+0x5,0xf2);break;
     case 7:outp(devport+0x5,0xf4);break;
     }
  outp(devport+0x4,0x80);
  prepare_dma(mixbuffer,65536);
  sendsb(0xd1);    //speaker(enable)
  sendsb(0x41);
  sendsb(samplerate>>8);
  sendsb(samplerate & 0xff); //Set_sample_rate(samplerate);
  sendsb(0xb4);
  sendsb(0x30);
  sendsb(i & 0xff);
  sendsb(i >> 8);  //play(16bit,signed,stereo,16384);
  outp(0x21,inp(0x21) & ~(1<<devirq));
 _dos_setvect(irqtable[devirq],sb16_irq);
  }

void stop_sb16()
  {
  sendsb(0xd3);    //speaker(disable)
  sendsb(0xd5);    //Halt_DMA_16bit()
  sendsb(0xd9);    //Exit_autoinit_16bit();
  sendsb(0xd5);    //Halt_DMA_16bit()
  if (devirq<8)
     {
     outp(0x21,inp(0x21) | (1<<devirq));
     }
  else
     {
     outp(0xA1,inp(0xA1) | (1<<(devirq-8)));
     }
  }



void start_sbstereo()
{
prepare_dma(mixbuffer,65536);
sendsb(0xd1);
sendsb(0x40);
sendsb((65536 - (256000000 / (samplerate*2)))>>8);
sendsb(0x48);
sendsb(0xff);
sendsb(0xff);
sendsb(0x90);
outp(devport+0x4,0x0e);
outp(devport+0x5,0x23-0x20*hw_intpol);
}

void __interrupt __far sb10_irq()
  {
  int i=32767;

  i=inp(devport+0xe);
  sendsb(0x14);
  sendsb(0xff);
  sendsb(0xff);
  outp(0x20,0x20);
  outp(0xA0,0x20);
  intr_test=1;
  }

void start_sb10()
  {
  prepare_dma(mixbuffer,65536);
  sendsb(0xd1);
  sendsb(0x40);
  sendsb(256 - (1000000 / samplerate));
  sendsb(0x14);
  sendsb(0xff);
  sendsb(0xff);
  outp(0x21,inp(0x21) & ~(1<<devirq));
 _dos_setvect(irqtable[devirq],sb10_irq);
  }

void stop_sb10()
  {
  sendsb(0xd3);    //speaker(disable)
  sendsb(0xd0);    //Halt_DMA_8bit()
  outp(0x21,inp(0x21) | (1<<devirq));
  }


void start_wss16()
  {
  int i,r,h;
  int wss_table[]=
     {8000,5513,16000,11025,27429,18900,32000,22050,-1,37800,-1,44100,48000,33075,9600,6615};

  int dma_table[]=
     {1,2,0,3};
  r=0xffffff;h=0;
  for(i=0;i<16;i++) if (wss_table[i]!=-1 && abs(samplerate-wss_table[i])<r)
                       {
                       h=i;r=abs(samplerate-wss_table[i]);
                       }
  outp(devport,dma_table[dmanum]);
  sendwss(0x09,0x0);
  outp(devport+0x6,0);
  sendwss(0x0a,0x3);
  samplerate=wss_table[h];
  sendwss(0x48,h+0x50);
  prepare_dma(mixbuffer,65536);
  sendwss(0x0e,0xff);
  sendwss(0x0f,0xff);
  sendwss(0x49,0x09);
  sendwss(0x09,0x09);
  }

void stop_wss16()
  {
  sendwss(0x49,0);
  sendwss(0x09,0);
  outp(devport+0x6,0);
  }

static int clear_buffer=0;

void set_backsnd_freq(int bfreq)
  {
  backstep=bfreq*0x10000/samplerate;
  }


int open_backsound(char *filename)
  {
  static char lastname[128];


  bblocks=10;
  if (filename!=NULL)
     {
     lastname[127]=0;
     strncpy(lastname,filename,127);
     }
  bsnd=fopen(lastname,"rb");
  if (bsnd==NULL)
     {
     clear_buffer=32;
     return -1;
     }
  fseek(bsnd,0,SEEK_END);
  blength=ftell(bsnd);
  fseek(bsnd,0,SEEK_SET);
  fread(&bchans,1,2,bsnd);
  fread(&bfreq,1,4,bsnd);
  fread(&bblocks,1,4,bsnd);
  fread(&bblocks,1,4,bsnd);
  fseek(bsnd,8,SEEK_CUR);
  fread(&btable,1,sizeof(btable),bsnd);
  set_backsnd_freq(bfreq);
  bblock_size=0;
  fread(&bblock_size,1,2,bsnd);
  fseek(bsnd,-2,SEEK_CUR);
  return 0;
  }


int load_music_block()
  {
  long remain;
  if (bsnd!=NULL && !clear_buffer)
     {
     fread(&remain,1,sizeof(remain),bsnd);
     fseek(bsnd,4,SEEK_CUR);
     fread(&depack,1,remain,bsnd);
     bblock_size=remain;
     }
  else
     {
     memset(&depack,0x80,sizeof(depack));
     remain=sizeof(depack);
     }
  return remain;
  }


int mix_back_sound(int synchro)
  {
  static int remain=0;
  static int last[2],xbass[2];
  static int swap=0,depos=0,nextpos=0;
  static int predstih=32768;
  int count,val,c;

  if (bvolume==0 && !clear_buffer) return 0;
  if (synchro==1)
     {
     remain=32767;
     depos=0;
     last[0]=0;
     last[1]=0;
     return nextpos;
     }
  if (synchro==2)
     {
     nextpos=backsnd*4+16;
     if (nextpos>BACK_BUFF_SIZE) nextpos-=BACK_BUFF_SIZE;
     return nextpos;
     }
  if (synchro==3)
     {
     nextpos=backsnd*4-16;
     if (nextpos<0) nextpos+=BACK_BUFF_SIZE;
     return nextpos;
     }
  if (synchro>=32768)
     {
     predstih=synchro;
     return nextpos;
     }
  for(;;)
     {
  val=backsnd*4-nextpos;
  if (val<0) val+=BACK_BUFF_SIZE;
  if (val<predstih) return nextpos;
  if (clear_buffer) clear_buffer--;
  for(count=4096;count>0;count--)
     {
     if (remain<=0)
       {
       bblocks--;
       if (bblocks<=0)
        {
        char *new_filename=NULL;
        fclose(bsnd);
        if (konec_skladby!=NULL) konec_skladby(&new_filename);
        open_backsound(new_filename);
        }
       last[0]=0;
       last[1]=0;
       remain=load_music_block();;
       depos=0;
       }
     val=btable[c=depack[depos]];
     if (bchans==2 || swap) remain--,depos++;
     val+=last[swap];
     last[swap++]=val;
     if (c==0)  //pridano jako provizorni reseni pro korekci chyby komprimacniho programu
        {
        val-=31767;
        }
     if (swap>1)swap=0;
     if (bxbass)
        {
        int c;
        xbass[swap]+=val;
        c=xbass[swap]>>XBASS_PARM;
        xbass[swap]-=c;
        c=(c*bxbass*4)>>8;
        val+=c;
        }
     val=(val*bvolume)>>8;
     if (val>32676) val=32767;
     if (val<-32767) val=-32767;
     *(short *)(backsndbuff+(nextpos))=val;
     nextpos+=2;
     if (nextpos>=BACK_BUFF_SIZE) nextpos=0;
     }
     }
  }


void fade_music()
  {
  short *p;
  int i,j,k,l,m;

  mix_back_sound(0);
  k=backsnd*2;
  i=k;
  p=(short *)&backsndbuff;
  m=mix_back_sound(1);
  m=m-k*2;
  if (m<0) m+=BACK_BUFF_SIZE;
  m/=2;
  j=m;m/=256;
  if (m)
  do
     {
     l=j/256;
     p[i]=p[i]*l/m;
     i++;j--;if (j<0) j=0;
     if (i>BACK_BUFF_SIZE/2) i=0;
     }
  while (i!=k);
  memset(&depack,0x80,sizeof(depack));
  }


void stop_sbstereo()
{
reset_blaster();
outp(devport+0x4,0x0e);
outp(devport+0x5,0x11);
//sendsb(0xda);
//sendsb(0xd0);
}

int init_blaster(int port)
{
devport=port;
reset_blaster();
return(inp(devport+0x0a)==0xaa);
}


void prepare_mixing(int mode)
{
predstih=samplerate/mixfreq+MIX_REZERVA;
predstih+=MIX_REZERVA+predstih;
predstih*=(mode/MIX_MONO);
switch (mode)
  {
  case MIX_MONO:setsb2();break;
  case MIX_MONO_S:setsb2_s();break;
  case MIX_STEREO:setsbpro();break;
  case MIX_16STEREO:setsb16();break;
  case MIX_ULTRA:setgus();break;
  }
//backsndbuff[0]=128;backstep=0;backsnd=0;
lastdma=0;
mixpos=(long)(mixbuffer+predstih);
memset(chaninfo,0,sizeof(TCHANNEL)*32);
memset(mixbuffer,0x80,65536);
}

void play_sample(int channel,void *sample,long size,long lstart,long sfreq,int type)
  {
  chaninfo[channel].play_pos=sample;
  chaninfo[channel].start_loop=(char *)sample+lstart;
  chaninfo[channel].end_loop=(char *)sample+size;
  chaninfo[channel].speed_maj=sfreq/samplerate;
  chaninfo[channel].speed_min=(sfreq%samplerate)*65536/samplerate;
  chaninfo[channel].sample_type=type;
  }

void chan_break_ext(int channel,void *org_sample,long size_sample)
  {
  chaninfo[channel].start_loop=chaninfo[channel].end_loop=(char *)org_sample+size_sample;
  }

void set_channel_volume(int channel,int left,int right)
  {
  left=(left*gfxvol)>>8;
  right=(right*gfxvol)>>8;
  left=max(left,0);left=min(left,32767);
  right=max(right,0);right=min(right,32767);
  if (swap_chans)
     {
     chaninfo[channel].vol_right=left;
     chaninfo[channel].vol_left=right;
     }
  else
     {
     chaninfo[channel].vol_left=left;
     chaninfo[channel].vol_right=right;
     }
  }


int int_relocation();
#pragma aux int_relocation modify [eax ebx ecx edx esi edi]
void int_mixer_alloc(int num);
#pragma aux int_mixer_alloc parm [ebx] modify [eax ecx edx esi edi]
int int_dealloc();
#pragma aux int_dealloc  modify [eax ebx ecx edx esi edi]
void high_speed_parm(void *mix,int dosdelay,char *xlattab,int port);
#pragma aux high_speed_parm parm [ebx][eax][esi][edx] modify [edi]
void int_high_alloc(int num);
#pragma aux int_high_alloc parm [ebx] modify [eax ecx edx esi edi]
void write_adlib(int data);
#pragma aux write_adlib parm[eax] modify [edx]
void write_adlib_nodelay(int data);
#pragma aux write_adlib_nodelay parm[eax] modify [edx]


void __interrupt __far int_nosound_counter()
  {
  test_counter++;
  _chain_intr(oldvect);
  }

void __interrupt __far int_adlib()
  {
  outp(0x20,0x20);
  _enable();
  if (inp(timer_test_port) & 0x80)
     {
     test_counter++;
     write_adlib_nodelay(0x8004);
     }
  mixer();
  _disable();
  _chain_intr(oldvect);
 }

void __interrupt __far int_normal()
  {
  outp(0x20,0x20);
  _enable();
  test_counter++;
  mixer();
  _disable();
  _chain_intr(oldvect);
  }


void int_init()
  {
  int l;

  l=0x1234bc/mixfreq;
  timer_test_port=0;
  _disable();
  oldvect=_dos_getvect(0x8);
  _dos_setvect(0x8,int_normal);
  outp(0x43,0x34);
  outp(0x40,l & 0xff);
  outp(0x40,l>>8);
  _enable();
  }

void int_init_adlib_timer()
  {
  int l;

  timer_value=l=255-3125/mixfreq;
  timer_test_port=0x388;
  _disable();
  oldvect=_dos_getvect(0x8);
  _dos_setvect(0x8,int_adlib);
  write_adlib(l*256+3);
  write_adlib(2*256+4);
  _enable();
  }
/*
void int_ultrasnd()
  {
  {
  int l;

  l=0x1234bc/mixfreq;
  timer_test_port=0;
  _disable();
  oldvect=_dos_getvect(0x8);
  _dos_setvect(0x8,gus_irq);
  outp(0x43,0x34);
  outp(0x40,l & 0xff);
  outp(0x40,l>>8);
  _enable();
  }
  }
*/
void int_nosound()
  {
  int l;
  _disable();
  oldvect=_dos_getvect(0x8);
  _dos_setvect(0x8,int_nosound_counter);
    l=0x1234bc/mixfreq;
  outp(0x43,0x34);
  outp(0x40,l%256);
  outp(0x40,l/256);
  _enable();
  }

void int_highspeed(int port)
  {
  int l;
  _dos_setvect(0x1c,int_normal);
  high_speed_parm(mixbuffer,samplerate/mixfreq,da_xlat,port);
  int_high_alloc(0x8);
  l=0x1234bc/samplerate;
  outp(0x43,0x34);
  outp(0x40,l%256);
  outp(0x40,l/256);
  }


void set_mixing_device(int mix_dev,int mix_freq,...)
  {
  int *p;
  samplerate=mix_freq;
  if (mixbuffer==NULL) mixbuffer=buff_alloc();
  p=&mix_freq;p++;
  device=mix_dev;
  switch (mix_dev)
     {
     case DEV_SB10:
     case DEV_SB20:
     case DEV_SBPRO:
     case DEV_SB16:
               init_blaster(p[0]);
               init_dma(p[1]);
               devirq=p[2];
               break;
     case DEV_DAC:
               if (p[0]==0x42) set_mixing_device(DEV_PCSPEAKER,mix_freq);
               else
                 {
                 rm_proc_set((long)mixbuffer>>4,dpmiselector,p[0],DAC_MODE);
                 if (samplerate>22050) samplerate=22050;
                 }
               break;
     case DEV_PCSPEAKER:
               rm_proc_set((long)mixbuffer>>4,dpmiselector,0x42,SPK_MODE);
               if (samplerate>19000) samplerate=19000;
               break;
     case DEV_ULTRA:
               init_dma(p[1]);
               devirq=p[2];
               dmamodenum=0x58;
               if (dmanum>3) gus_dma_type=0x45;else gus_dma_type=0x41;
               break;
     case DEV_WSS:
               devport=p[0];
               init_dma(p[1]);
               break;
     }
  }

long pc_speak_position(void);

void start_mixing()
  {
  ticker_save=*(long *)0x46c;
  test_counter=0;
  switch (device)
     {
     case DEV_SB10:
              prepare_mixing(MIX_MONO);
              start_sb10();
              int_init();
              break;
     case DEV_SB20:
              prepare_mixing(MIX_MONO);
              start_sbmono();
              int_init();
              break;
     case DEV_SBPRO:
              prepare_mixing(MIX_STEREO);
              start_sbstereo();
              int_init();
              break;
     case DEV_SB16:
              prepare_mixing(MIX_16STEREO);
              start_sb16();
              int_init();
              break;
     case DEV_PCSPEAKER:
              prepare_mixing(MIX_MONO_S);
              pc_speak_set_proc(&getdma);
              load_rm_proc();
              int_init();
              pc_speak_enable();
              pc_speak_run(samplerate,mixfreq);
              break;
     case DEV_DAC:
              prepare_mixing(MIX_MONO_S);
              pc_speak_set_proc(&getdma);
              load_rm_proc();
              int_init();
              pc_speak_run(samplerate,mixfreq);
              break;
     case DEV_ULTRA:
              prepare_mixing(MIX_ULTRA);
              init_gus();
              int_init();
              delay(200);
              break;
     case DEV_WSS:
              prepare_mixing(MIX_16STEREO);
              start_wss16();
              int_init();
              break;
     case DEV_NOSOUND:int_nosound();break;
     }
  mixer_zavora=0;
  }

void stop_mixing()
  {
  mixer_zavora=1;
  _disable();
  *(long *)0x46c=ticker_save+(long)(((float)test_counter/mixfreq)*(18.2059));
  outp(0x43,0x34);
  outp(0x40,0);
  outp(0x40,0);
  _enable();
  switch (device)
     {
     case DEV_SB10:
              stop_sb10();
              break;
     case DEV_SB20:
              stop_sbmono();
              break;
     case DEV_SBPRO:
              stop_sbstereo();
              break;
     case DEV_SB16:
              stop_sb16();
              break;
     case DEV_PCSPEAKER:
              pc_speak_stop();
              pc_speak_disable();
              purge_rm_proc();
              break;
     case DEV_DAC:
              pc_speak_stop();
              purge_rm_proc();
              break;
     case DEV_ULTRA:
              stop_gus();
              break;
     case DEV_WSS:
              stop_wss16();
              break;
     }
  _dos_setvect(0x8,oldvect);
  }

void __far __interrupt sb_detect_irq2()
irqdetect(2)
void __far __interrupt sb_detect_irq3()
irqdetect(3)
void __far __interrupt sb_detect_irq5()
irqdetect(5)
void __far __interrupt sb_detect_irq7()
irqdetect(7)


char *device_name(int device)
  {

  switch (device)
     {
     case DEV_NOSOUND:return names[1];
     case DEV_SB10:return names[2];
     case DEV_SB20:return names[3];
     case DEV_SBPRO:return names[4];
     case DEV_SB16:return names[5];
     case DEV_WSS:return names[6];
     case DEV_ULTRA:return names[7];
     case DEV_DAC:return names[8];
     case DEV_PCSPEAKER:return names[9];
     }
  return names[0];
  }

int sb_detect(int *dev,int *port,int *dma, int *irq)
  {
  int i,a,j;
  char ver_lo;
  char ver_hi;
  void __far *irqsave2,__far *irqsave3,__far *irqsave5,__far *irqsave7;
  int dmasaves[4];
  char dmadis[4];
  int dmatesting[4]={1,3,0,1};

  *port=0x210;
  do
     {
     devport=*port;
     outp(devport+6,1);delay(1);
     outp(devport+6,0);
     for(i=0;i<100 && inp(devport+0xA)!=0xAA;i++) delay(1);
     (*port)+=0x10;
     }
  while (i==100 && devport<0x280);
  if (i==100) return -1; //NOT DETECTED;
  *port=devport;
  _disable();sendsb(0xe1);recivesb(ver_lo);recivesb(ver_hi);_enable();
  switch (ver_hi)
     {
     case 1: *dev=DEV_SB10;break;
     case 2: if (ver_lo) *dev=DEV_SB20;break;
     case 3: *dev=DEV_SBPRO;break;
     case 4: *dev=DEV_SB16;break;
     default: *dev=DEV_SB16;break;
     }
  if (*dev==DEV_SB16)
     {
     outp(devport+4,0x80);
     i=inp(devport+5);
     *irq=5;
     if (i & 8) *irq=7;
     if (i & 4) *irq=7;
     if (i & 2) *irq=5;
     if (i & 1) *irq=2;
     outp(devport+4,0x81);
     i=inp(devport+5);
     *dma=1;
     if (i & 0x20) *dma=5;
     else if (i & 0x40) *dma=6;
     else if (i & 0x80) *dma=7;
     else if (i & 1) *dma=0;
     else if (i & 2) *dma=1;
     else if (i & 8) *dma=3;
     return 0;
     }
  for(i=0;i<4;i++)
     {
     outp(0xc,0);
     a=inp(dmatable[i][0]);
     a=a+256*inp(dmatable[i][0]);
     dmasaves[i]=a;
     }
  irqsave2=_dos_getvect(irqtable[2]);
  irqsave3=_dos_getvect(irqtable[3]);
  irqsave5=_dos_getvect(irqtable[5]);
  irqsave7=_dos_getvect(irqtable[7]);
  _dos_setvect(irqtable[2],sb_detect_irq2);
  _dos_setvect(irqtable[3],sb_detect_irq3);
  _dos_setvect(irqtable[5],sb_detect_irq5);
  _dos_setvect(irqtable[7],sb_detect_irq7);
  ver_lo=inp(0x21);
  outp(0x21,0x53);
  memset(detect_enables,0x1,sizeof(detect_enables));
  delay(100);
  devirq=-1;
  sendsb(0xf2);
  delay(1);
  _dos_setvect(irqtable[2],irqsave2);
  _dos_setvect(irqtable[3],irqsave3);
  _dos_setvect(irqtable[5],irqsave5);
  _dos_setvect(irqtable[7],irqsave7);
  outp(0x21,ver_lo);
  if (devirq==-1) return -2; //DETECTION ERROR
  *irq=devirq;
  for(i=0;i<4;i++)
     {
     outp(0xc,0);
     a=inp(dmatable[i][0]);
     a=a+256*inp(dmatable[i][0]);
     dmadis[i]=(dmasaves[i]==a);
     }
  for (j=0,*dma=-1;j<4 && *dma==-1;j++)
  {
  i=dmatesting[j];
  init_dma(i);
  prepare_dma(NULL,65536);
  sendsb(0xd3);
  sendsb(0x14);
  sendsb(0xff);
  sendsb(0x7f);
  delay(100);
  outp(0xc,0);
  a=inp(dmatable[i][0]);
  a=a+256*inp(dmatable[i][0]);
  if (dmasaves[i]!=a && dmadis[i]) *dma=i;
  reset_blaster();
     }
  if (*dma==-1) return -2;
  return 0;
  }

int wss_detect(int *dev,int *port,int *dma,int *irq)
  {
  int wss_ports[]={0x530,0x604,0xe80,0xf40};
  int i,d;

  for(i=0;i<4;i++)
     {
     devport=wss_ports[i];
     if (!(inp(devport+0x4) & 0x80))
        {
        sendwss(00,0xaa);
        recivewss(00,d)
        if (d==0xAA) break;
        }
     }
  if (i==4) return -1;
  *dev=DEV_WSS;
  *port=devport;
  *dma=1;
  *irq=0;
  return 0;
  }

int sound_detect(int *dev,int *port,int *dma, int *irq)
  {
  char *env;
  char *f;

  *dev=0;
  env=getenv("BS_SOUND");
  if (env!=NULL)
     if (sscanf(env,"%d %x %d %d",dev,port,dma,irq)!=4) *dev=0;
  if (*dev==0)
     if (wss_detect(dev,port,dma,irq)) *dev=0;
  if (*dev==0 && (env=getenv("ULTRASND"))!=NULL)
     {
     char c;int i;
     sscanf(env,"%x%c%d%c%d%c%d",port,&c,dma,&c,&i,&c,irq);
     *dev=DEV_ULTRA;
     }
  if (*dev==0 && (env=getenv("BLASTER"))!=NULL)
     {
     int i;
     f=strchr(env,'T');
     if (f!=NULL)
        {
        sscanf(f+1,"%d",&i);
        switch (i)
           {
           case 1:*dev=DEV_SB10;break;
           case 2:*dev=DEV_SBPRO;break;
           case 3:*dev=DEV_SB20;break;
           case 4:*dev=DEV_SBPRO;break;
           default: *dev=DEV_NOSOUND;break;
           }
        if (i>4) *dev=DEV_SB16;
        }
     f=strchr(env,'A');
     if (f!=NULL) sscanf(f+1,"%x",port);else *dev=0;
     f=strchr(env,'I');
     if (f!=NULL) sscanf(f+1,"%d",irq);else *dev=0;
     f=strchr(env,'D');
     if (f!=NULL) sscanf(f+1,"%d",dma);else *dev=0;
     f=strchr(env,'H');
     if (f!=NULL) sscanf(f+1,"%d",dma);
     }
  if (*dev==0)
     {
     if (sb_detect(dev,port,dma,irq) && sb_detect(dev,port,dma,irq) && sb_detect(dev,port,dma,irq))
        {
        *dev=0;
        }
     }
  return -(*dev==0);
  }

/*main()
{
printf("%d\n",sizeof(TCHANNEL));
if (sb_detect(&a,&b,&c,&d))
  {
  printf("SB not present\n");
  return;
  }
//a=DEV_SBPRO;
printf("%s\n",device_name(a));
//printf("IRQ:\n");
//scanf("%d",&d);
set_mixing_device(a,22050,b,c,d);
printf("Port %03X dma %d irq %d freq %d.\n",b,c,d,samplerate);
open_backsound(MUSIC1);
start_mixing();
mixing();
stop_mixing();
printf("\n\n\n");
}
*/
void change_music(char *filename)
  {
        mix_back_sound(0);
        if (bsnd!=NULL)fclose(bsnd);
        fade_music();
        open_backsound(filename);
  }

int get_timer_value()
  {
  return test_counter;
  }

char get_channel_state(int channel)
  {
  return chaninfo[channel].sample_type;
  }

void get_channel_volume(int channel,int *left,int *right)
  {
  *left=chaninfo[channel].vol_left;
  *right=chaninfo[channel].vol_right;
  }

void mute_channel(int channel)
  {
  chaninfo[channel].sample_type=0;
  }

void chan_break_loop(int channel)
  {
  chaninfo[channel].start_loop=chaninfo[channel].end_loop;
  }

void get_bass_treble(int *x,int *y)
  {
  if (device==DEV_SB16)
     {
     outp(devport+4,0x44);*x=inp(devport+5);
     outp(devport+4,0x46);*y=inp(devport+5);
     }
  else
     {
     *x=255;
     *y=255;
     }
  }

char sb_set_global_volume(int x)
  {
  if (device==DEV_SBPRO)
     {
     outp(devport+4,2);outp(devport+5,(x & 0xf0) | (x>>4));
     }
  else if (device==DEV_SB16)
     {
     outp(devport+4,2);outp(devport+5,(x & 0xf0) | (x>>4));
     outp(devport+4,0x30);outp(devport+5,x);
     outp(devport+4,0x31);outp(devport+5,x);
     }
  else return 1;
  return 0;
  }


char sb_get_global_volume()
  {
  if (device==DEV_SBPRO)
     {
     outp(devport+4,2);return inp(devport+5)&0xf0;
     }
  else if (device==DEV_SB16)
     {
     int check1,check2;
     outp(devport+4,2);check1=inp(devport+5)&0xf0;
     outp(devport+4,0x30);check2=inp(devport+5);
     if (check1!=check2 & 0xe0) return check1;else return check2;
     }
  else return 1;
  }

void wss_set_global_volume(int x)
  {
  x>>=4;
  sendwss(06,0xf-x);
  sendwss(07,0xf-x);
  }

char wss_get_global_volume()
  {
  int x;

  recivewss(06,x);
  x<<=4;
  return 0xff-x;
  }

void set_music_volume(int volume)
  {
  if (volume==0 && bvolume!=0)
     memset(backsndbuff,0,sizeof(backsndbuff));
  bvolume=volume;
  }

int get_music_volume()
  {
  return bvolume;
  }

void set_gfx_volume(int volume)
  {
  gfxvol=volume;
  }

int get_gfx_volume()
  {
  return gfxvol;
  }


void snd_ping1(int data)
  {
  data;
  }

int snd_ping2()
  {
  return 0;
  }


void set_swap_channels(int data)
  {
  swap_chans=data & 1;
  }

int get_swap_channels()
  {
  return swap_chans;
  }

void set_treble(int y)
  {
  outp(devport+4,0x44);outp(devport+5,y);
  outp(devport+4,0x45);outp(devport+5,y);
  }
void set_bass(int x)
  {
  outp(devport+4,0x46);outp(devport+5,x);
  outp(devport+4,0x47);outp(devport+5,x);
  }

int get_treble()
  {
  outp(devport+4,0x44);return inp(devport+5);
  }
int get_bass()
  {
  outp(devport+4,0x46);return inp(devport+5);
  }

char gus_swap=0;
char gus_volume=0xff;

void set_gus_swap(int x)
  {
  gus_swap=x;
  gus_setchan_pan(0,x>>4);
  gus_setchan_pan(1,(0xff-x)>>4);
  }

void set_xbass(int x)
  {
  bxbass=x;
  }

int get_xbass()
  {
  return bxbass;
  }

int get_gus_swap()
  {
  return gus_swap;
  }

void gus_set_global_volume(int x)
  {
  gus_setchan_vol(0,x<<8);
  gus_setchan_vol(1,x<<8);
  gus_volume=x;
  }

int gus_get_global_volume()
  {
  return gus_volume;
  }

char get_outfilter()
  {
  return hw_intpol;
  }


char set_outfilter(char value)
  {
  int a;

  if (device!=DEV_SBPRO) return 1;
  if (value>1) hw_intpol=!hw_intpol;
  else hw_intpol=value;
   do
   {
   a=inp(dmaposadr);
   a+=inp(dmaposadr)<<8;
   }
   while (a & 1);
   do
   {
   a=inp(dmaposadr);
   a+=inp(dmaposadr)<<8;
   }
   while (!(a & 1));
   outp(devport+0x4,0x0e);
   if (hw_intpol) outp(devport+0x5,inp(devport+0x5) & ~0x20);
   else outp(devport+0x5,inp(devport+0x5) | 0x20);
  return 0;
  }

typedef void (*FPROC_SET)(int data);
typedef int (*FPROC_GET)();

FPROC_SET set_funct_table[][SND_MAXFUNCT]=
  {
  {snd_ping1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {snd_ping1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,sb_set_global_volume,NULL,NULL,set_swap_channels,NULL,NULL,set_outfilter,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,sb_set_global_volume,set_bass,set_treble,set_swap_channels,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,wss_set_global_volume,NULL,NULL,set_swap_channels,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,gus_set_global_volume,NULL,NULL,NULL,set_gus_swap,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  {snd_ping1,NULL,NULL,NULL,NULL,NULL,NULL,NULL,set_gfx_volume,set_music_volume,set_xbass},
  };

FPROC_GET get_funct_table[][SND_MAXFUNCT]=
  {
  {snd_ping2,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {snd_ping2,NULL,NULL,NULL,NULL,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,NULL,NULL,NULL,NULL,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,sb_get_global_volume,NULL,NULL,get_swap_channels,NULL,NULL,get_outfilter,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,sb_get_global_volume,get_bass,get_treble,get_swap_channels,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,wss_get_global_volume,NULL,NULL,get_swap_channels,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,gus_get_global_volume,NULL,NULL,NULL,get_gus_swap,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,NULL,NULL,NULL,NULL,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  {snd_ping2,NULL,NULL,NULL,NULL,NULL,NULL,NULL,get_gfx_volume,get_music_volume,get_xbass},
  };

char check_snd_effect(int funct)
  {
  return set_funct_table[device][funct]!=NULL;
  }

char set_snd_effect(int funct,int data)
  {
  if (set_funct_table[device][funct]!=NULL)
     {
     set_funct_table[device][funct](data);
     return 1;
     }
  return 0;
  }

int get_snd_effect(int funct)
  {
  if (get_funct_table[device][funct]!=NULL)
     {
     return get_funct_table[device][funct]();
     }
  return 0;
  }


void force_music_volume(int volume)
  {
  bvolume=volume;
  }
