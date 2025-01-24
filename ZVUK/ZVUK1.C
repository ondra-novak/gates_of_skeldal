#include <stdlib.h>
#include <stdio.h>
//#include <i86.h>
#include <dpmi.h>
#include <conio.h>
#include <mem.h>
#include <bios.h>
#include <dos.h>


#define MIX_MONO 8
#define MIX_STEREO 16
#define MIX_16STEREO 32

#define MUSIC1 "..\\music\\ascent.mus"
#define MUSIC2 "..\\music\\late.mus"
#define MUSIC3 "..\\music\\december.mus"
#define MUSIC4 "..\\music\\ssi.mus"
#define MUSIC5 "..\\music\\test.mus"


#define BACK_BUFF_SIZE 0x40000

#define DEV_SB10 0x0e
#define DEV_SB20 0x10
#define DEV_SBPRO 0x12
#define DEV_SB16 0x14
#define DEV_WWS 0x16
#define DEV_ULTRA 0x18
#define DEV_DAC 0x1A
#define DEV_PCSPEAKER 0x1C

#define MIX_REZERVA 0x50

#define build_dac_xlat() \
                       {\
                          int i;\
                          for(i=0;i<256;i++) da_xlat[i]=i;\
                       }\

#define sendsb(data) \
                 while (inp(sbport+0xc) & 0x80);\
                 outp(sbport+0xc,data)

#define recivesb(data)\
                 while (!(inp(sbport+0xe) & 0x80));\
                 data=inp(sbport+0xa)

char detect_enables[8];

#define irqdetect(irq) \
                 {\
                 if (detect_enables[irq]) \
                    sbirq=irq;\
                 detect_enables[irq]=0;\
                 inp(sbport+0xE);\
                 outp(0x20,0x20);\
                 }


typedef struct tchannel
  {
  char *play_pos, *start_loop, *end_loop;
  long speed_maj;
  unsigned short speed_min, minor_pos, sample_type, vol_left, vol_right;
  }TCHANNEL;

char musiclist[5][30]={
                 MUSIC1,MUSIC2,MUSIC3,MUSIC4,MUSIC5};

TCHANNEL chaninfo[32];
char *mixbuffer;
char backsndbuff[BACK_BUFF_SIZE];
volatile long backsnd=0;
volatile long backstep=1;
volatile int  backfine=0;
long jumptable[3];
long getdma;
long ido;
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
int device;
long samplerate=1000;
long mixfreq=100;
long idt_map[2];
int call_back_data;
int call_back_sel;
unsigned short dpmiselector;
char mixer_zavora=0;
FILE *bsnd;
unsigned short bchans;
short btable[256];
int bfreq,bblocks,bvolume=255;
char depack[32768];

char da_xlat[256]; // Xlat tabulka pro DA a PC Speaker;
int dac_port;    //DAC port
int test_counter=0;

char *sample1;
long last_load_size;

int dmatable[8][4]=
  {
  {0,1,0x87,0x0A0B},
  {2,3,0x83,0x0A0B},
  {4,5,0x81,0x0A0B},
  {6,7,0x82,0x0A0B},
  {0xd0,0xd1,0x8f,0xD4D6},
  {0xd2,0xd3,0x8b,0xD4D6},
  {0xd4,0xd5,0x89,0xD4D6},
  {0xd6,0xd7,0x8a,0xD4D6}
  };

int irqtable[16]={8,9,0xa,0xb,0xc,0xd,0xe,0xf,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77};

void *load_file(char *filename)
  {
  FILE *f;
  long size,*p;

  f=fopen(filename,"rb");
  if (f==NULL) abort();
  fseek(f,0,SEEK_END);
  size=ftell(f);
  fseek(f,0,SEEK_SET);
  p=(void *)malloc(size);
  if (fread(p,1,size,f)!=size) abort;
  fclose(f);
  last_load_size=size;
  return p;
  }

char *buff_alloc();
#pragma aux buff_alloc modify[ebx ecx edx] value [eax]
void mixer();
#pragma aux mixer modify [eax ebx ecx edx esi edi]
void setsbpro();
#pragma aux setsbpro modify [eax edi]
void setsb2();
#pragma aux setsb2 modify [eax edi]
void setsb16();
#pragma aux setsb16 modify [eax edi]

int sbport=0x220;
int sbirq=0;

void init_dma(int dma)
{
dmanum=dma;
dmaposadr=dmatable[dma][0];
dmasizadr=dmatable[dma][1];
dmapageadr=dmatable[dma][2];
dmamask=dmatable[dma][3] >> 8;
dmamode=dmatable[dma][3] & 0xff;
}

void prepare_dma(char *adr)
{
int poz;

outp(0x0C,0);outp(0xD8,0);
outp(dmamask,(dmanum & 3)+0x04);
outp(dmamode,(dmanum & 3)+0x58);
if (dmanum>3) poz=(long)adr >> 1; else poz=(long)adr;
outp(dmaposadr,poz & 0xff);
outp(dmaposadr,(poz>>8) & 0xff);
outp(dmapageadr,poz>>16);
outp(dmasizadr,0xff);
outp(dmasizadr,0xff);
outp(dmamask,dmanum & 3+0x04);
}



void reset_blaster()
{
int counter=100;
outp(sbport+0x6,1);
delay(10);
outp(sbport+0x6,0);
while (inp(sbport+0x0A)!=0xAA && counter--) delay(1);
}
void start_sbmono()
{
prepare_dma(mixbuffer);
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

  i=inp(sbport+0xf);
  i=inp(sbport+0xe);
//  sendsb(0xb6);
//  sendsb(0x30);
//  sendsb(i & 0xff);
//  sendsb(i >> 8);  //play(16bit,signed,stereo,16384);
  outp(0x20,0x20);
  outp(0xA0,0x20);
  }
void start_sb16()
  {
  int i=16383;

  outp(sbport+0x4,0x81);
  outp(sbport+0x5,1 << dmanum);
  outp(sbport+0x4,0x80);
  switch (sbirq)
     {
     case 2:outp(sbport+0x5,0xf1);break;
     case 5:outp(sbport+0x5,0xf2);break;
     case 7:outp(sbport+0x5,0xf4);break;
     }
  outp(sbport+0x4,0x80);
  prepare_dma(mixbuffer);
  sendsb(0xd1);    //speaker(enable)
  sendsb(0x41);
  sendsb(samplerate>>8);
  sendsb(samplerate & 0xff); //Set_sample_rate(samplerate);
  sendsb(0xb4);
  sendsb(0x30);
  sendsb(i & 0xff);
  sendsb(i >> 8);  //play(16bit,signed,stereo,16384);
  outp(0x21,inp(0x21) & ~(1<<sbirq));
 _dos_setvect(irqtable[sbirq],sb16_irq);
  }

void stop_sb16()
  {
  sendsb(0xd3);    //speaker(disable)
  sendsb(0xd5);    //Halt_DMA_16bit()
  sendsb(0xd9);    //Exit_autoinit_16bit();
  sendsb(0xd5);    //Halt_DMA_16bit()
  if (sbirq<8)
     {
     outp(0x21,inp(0x21) | (1<<sbirq));
     }
  else
     {
     outp(0xA1,inp(0xA1) | (1<<(sbirq-8)));
     }
  }



void start_sbstereo()
{
prepare_dma(mixbuffer);
sendsb(0xd1);
sendsb(0x40);
sendsb((65536 - (256000000 / (samplerate*2)))>>8);
sendsb(0x48);
sendsb(0xff);
sendsb(0xff);
sendsb(0x90);
outp(sbport+0x4,0x0e);
outp(sbport+0x5,0x13);
}

void __interrupt __far sb10_irq()
  {
  int i=32767;

  i=inp(sbport+0xe);
  sendsb(0x14);
  sendsb(0xff);
  sendsb(0xff);
  outp(0x20,0x20);
  outp(0xA0,0x20);
  }

void start_sb10()
  {
  prepare_dma(mixbuffer);
  sendsb(0xd1);
  sendsb(0x40);
  sendsb((65536 - (256000000 / (samplerate*2)))>>8);
  sendsb(0x14);
  sendsb(0xff);
  sendsb(0xff);
  outp(0x21,inp(0x21) & ~(1<<sbirq));
 _dos_setvect(irqtable[sbirq],sb10_irq);
  }

void stop_sb10()
  {
  sendsb(0xd3);    //speaker(disable)
  sendsb(0xd5);    //Halt_DMA_16bit()
  if (sbirq<8)
     {
     outp(0x21,inp(0x21) | (1<<sbirq));
     }
  else
     {
     outp(0xA1,inp(0xA1) | (1<<(sbirq-8)));
     }
  }

int open_backsound(char *filename)
  {
  static char lastname[128];


  if (filename!=NULL)
     {
     lastname[127]=0;
     strncpy(lastname,filename,127);
     }
  bsnd=fopen(lastname,"rb");
  if (bsnd==NULL) return -1;
  fread(&bchans,1,2,bsnd);
  fread(&bfreq,1,4,bsnd);
  fread(&bblocks,1,4,bsnd);
  fread(&bblocks,1,4,bsnd);
  fseek(bsnd,8,SEEK_CUR);
  fread(&btable,1,sizeof(btable),bsnd);
  return 0;
  }


void load_music_block()
  {
  if (bsnd!=NULL)
     {
     fseek(bsnd,8,SEEK_CUR);
     fread(&depack,1,sizeof(depack),bsnd);
     }
  else
     {
     memset(&depack,0x80,sizeof(depack));
     bblocks=0;
     }
  }


int mix_back_sound(char synchro)
  {
  static int remain=0;
  static int last[2];
  static int swap=0,depos=0,nextpos=0;
  int count,val;

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
  for(;;)
     {
  val=backsnd*4-nextpos;
  if (val<0) val+=BACK_BUFF_SIZE;
  if (val<32768) return nextpos;
  for(count=16384;count>0;count--)
     {
     if (remain==0)
       {
       bblocks--;
       if (bblocks<=0)
        {
        fclose(bsnd);
        open_backsound(NULL);
        }
       load_music_block();
       last[0]=0;
       last[1]=0;
       remain=32768;
       depos=0;
       }
     val=btable[depack[depos++]];remain--;
     val+=last[swap];
     last[swap++]=val;
     if (swap>=bchans) swap=0;
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
outp(sbport+0x4,0x0e);
outp(sbport+0x5,0x11);
//sendsb(0xda);
//sendsb(0xd0);
}

int init_blaster(int port)
{
sbport=port;
reset_blaster();
return(inp(sbport+0x0a)==0xaa);
}


void prepare_mixing(int mode)
{
predstih=samplerate/mixfreq+MIX_REZERVA;
predstih+=MIX_REZERVA+predstih;
predstih*=(mode/MIX_MONO);
switch (mode)
  {
  case MIX_MONO:setsb2();break;
  case MIX_STEREO:setsbpro();break;
  case MIX_16STEREO:setsb16();break;
  }
backsndbuff[0]=0;backstep=bfreq*0x10000/samplerate;backsnd=0;
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

void set_channel_volume(int channel,int left,int right)
  {
  chaninfo[channel].vol_left=left;
  chaninfo[channel].vol_right=right;
  }

void set_bass_treble(int x,int y)
  {
  outp(sbport+4,0x44);outp(sbport+5,y);
  outp(sbport+4,0x45);outp(sbport+5,y);
  outp(sbport+4,0x46);outp(sbport+5,x);
  outp(sbport+4,0x47);outp(sbport+5,x);
  }

void mixing()
{
int c;int smpsize;
int treble=240,bass=240;

sample1=(char *)load_file("d:\\music\\samples\\fx\\playboy.smp");
smpsize=last_load_size;
set_channel_volume(0,32768,0);
set_channel_volume(1,0,32768);
for (c=2;c<12;c++)
     set_channel_volume(c,(c-2)*3000,27000-(c-2)*3000);
c=0;
while (c!=1)
{
int a,b;
  a=inp(dmaposadr);
  a+=inp(dmaposadr)<<8;
  b=(bblocks*65536)/(22050*bchans*2);
  cprintf("%04X %6X %6X volume %03d block:%04d time %02d:%02d  %10d\r",a,mixpos,mixsize,bvolume,bblocks,b/60,b%60,test_counter);
  mix_back_sound(0);
  //mixer();
  c=0;
  while (_bios_keybrd(_KEYBRD_READY))
     {
     c=_bios_keybrd(_KEYBRD_READ)>>8;
     switch(c)
        {
        case 'H': if (bvolume<512) bvolume++;break;
        case 'P': if (bvolume>0) bvolume--;break;
        case 'I': if (treble<240) treble+=16;set_bass_treble(bass,treble);break;
        case 'Q': if (treble>0) treble-=16;set_bass_treble(bass,treble);break;
        case 'G': if (bass<240) bass+=16;set_bass_treble(bass,treble);break;
        case 'O': if (bass>0) bass-=16;set_bass_treble(bass,treble);break;
        case 'M': if (bblocks>0)
              {
              mixer_zavora=1;
              backsnd+=8196;if (backsnd>BACK_BUFF_SIZE/4) backsnd-=BACK_BUFF_SIZE/4;
              mixer_zavora=0;
              }
              break;
        case 'K': if (ftell(bsnd)>2*(32768+8))
                 {
                 fseek(bsnd,-2*(32768+8),SEEK_CUR);
                 bblocks++;
                 load_music_block();
                 }
                 break;
        case 0x17:
              do
              {
              a=inp(dmaposadr);
              a+=inp(dmaposadr)<<8;
              }
              while (!(a & 1));
              do
              {
              a=inp(dmaposadr);
              a+=inp(dmaposadr)<<8;
              }
              while (a & 1);
              outp(sbport+0x4,0x0e);
              outp(sbport+0x5,inp(sbport+0x5) ^ 0x20);
              break;
        default:
        if (c>=2 && c<=11)
           play_sample(c-2,sample1,smpsize,smpsize,11000,1);
           else
        if (c>=';' && c<='@')
        {
        fclose(bsnd);
        fade_music();
        open_backsound(musiclist[c-';']);
        }

        }
     }
  }
}

int int_relocation();
#pragma aux int_relocation modify [eax ebx ecx edx esi edi]
void int_mixer_alloc(int num);
#pragma aux int_mixer_alloc parm [ebx] modify [eax ecx edx esi edi]
int int_dealloc();
#pragma aux int_dealloc  modify [eax ebx ecx edx esi edi]
void __interrupt __far int_normal();
void high_speed_parm(void *mix,int dosdelay,char *xlattab,int port);
#pragma aux high_speed_parm parm [ebx][eax][esi][edx] modify [edi]
void int_high_alloc(int num);
#pragma aux int_high_alloc parm [ebx] modify [eax ecx edx esi edi]


void int_init()
  {
  int l;
  _dos_setvect(0x1c,int_normal);
  l=0x1234bc/mixfreq;
  outp(0x43,0x34);
  outp(0x40,l%256);
  outp(0x40,l/256);
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
  mixbuffer=buff_alloc();
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
               sbirq=p[2];
               break;
     case DEV_DAC:
               dac_port=p[0];
               break;
     }
  }

void start_mixing()
  {
  switch (device)
     {
     case DEV_SB10:
              start_sb10();
              int_init();
              prepare_mixing(MIX_MONO);
              break;
     case DEV_SB20:
              start_sbmono();
              int_init();
              prepare_mixing(MIX_MONO);
              break;
     case DEV_SBPRO:
              start_sbstereo();
              int_init();
              prepare_mixing(MIX_STEREO);
              break;
     case DEV_SB16:
              start_sb16();
              int_init();
              prepare_mixing(MIX_16STEREO);
              break;
     case DEV_DAC:
              build_dac_xlat();
              prepare_mixing(MIX_MONO);
              int_highspeed(dac_port);
              break;
     }
  }

void stop_mixing()
  {
  outp(0x43,0x34);
  outp(0x40,0);
  outp(0x40,0);
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
     case DEV_DAC:
              int_dealloc();
              break;
     }
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
  static char names[][25]=
     {
     "Unsupported Device",
     "Sound Blaster",
     "Sound Blaster 2",
     "Sound Blaster Pro/2",
     "Sound Blaster 16+",
     "Windows Sound System",
     "UltraSound",
     "DAC on LPT",
     "Internal Hooker",
     "Pro Audio Spectrum"
     };

  switch (device)
     {
     case DEV_SB10:return names[1];
     case DEV_SB20:return names[2];
     case DEV_SBPRO:return names[3];
     case DEV_SB16:return names[4];
     case DEV_WWS:return names[5];
     case DEV_ULTRA:return names[6];
     case DEV_DAC:return names[7];
     case DEV_PCSPEAKER:return names[8];
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
     sbport=*port;
     outp(sbport+6,1);delay(1);
     outp(sbport+6,0);
     for(i=0;i<100 && inp(sbport+0xA)!=0xAA;i++) delay(1);
     (*port)+=0x10;
     }
  while (i==100 && sbport<0x280);
  if (i==100) return -1; //NOT DETECTED;
  *port=sbport;
  sendsb(0xe1);
  recivesb(ver_lo);
  recivesb(ver_hi);
  switch (ver_hi)
     {
     case 1: *dev=DEV_SB10;break;
     case 2: if (ver_lo) *dev=DEV_SB20;break;
     case 3: *dev=DEV_SBPRO;break;
     case 4: *dev=DEV_SB16;break;
     default: *dev=DEV_SB10;break;
     }
  if (*dev==DEV_SB16)
     {
     outp(sbport+4,0x80);
     i=inp(sbport+5);
     *irq=5;
     if (i & 8) *irq=7;
     if (i & 4) *irq=7;
     if (i & 2) *irq=5;
     if (i & 1) *irq=2;
     outp(sbport+4,0x81);
     i=inp(sbport+5);
     *dma=1;
     if (i & 1) *dma=0;
     if (i & 2) *dma=1;
     if (i & 8) *dma=3;
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
  sbirq=-1;
  sendsb(0xf2);
  delay(1);
  _dos_setvect(irqtable[2],irqsave2);
  _dos_setvect(irqtable[3],irqsave3);
  _dos_setvect(irqtable[5],irqsave5);
  _dos_setvect(irqtable[7],irqsave7);
  outp(0x21,ver_lo);
  if (sbirq==-1) return -2; //DETECTION ERROR
  *irq=sbirq;
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
  prepare_dma(NULL);
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

main()
{
int a,b,c,d;
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




