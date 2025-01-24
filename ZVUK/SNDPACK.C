#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>

#define BLK_SIZE 8192

short ampl_table[256];
signed short source[BLK_SIZE];
char target[BLK_SIZE];

long sfreq;
unsigned short chans;
FILE *sf,*tf;


char *nsource;
char *ntarget;
long ssize;
long rsize,blck;
char difftype=4;

void Create_table_16()
  {
   int a,c,d,e;
   float b;

   d=-1;e=0;
   for(a=0;a<128;a++)
     {
     b=(a/128.0);
     switch (difftype)
        {
        case 1:b=(b*32768.0);break;
        case 2:b=(b*b*32768.0);break;
        case 3:b=(b*b*b*32768.0);break;
        case 4:b=(b*b*b*b*32768.0);break;
        case 5:b=(b*b*b*b*b*32768.0);break;
        }
     c=(int)b;
     if (c==d) e++;
     d=c;c+=e;
     ampl_table[128+a]=c;
     ampl_table[128-a]=-c;
     }
  }

int find_index(int value)
  {
  int i;

  if (value==0) return 128;
  if (value>0)
     {
     for(i=128;i<256;i++)
        if (ampl_table[i]>value) return i-1;
     return 255;
     }
  else
     {
     for(i=128;i>=0;i--)
        if (ampl_table[i]<value) return i+1;
     return 1;
     }
  }

void compress_buffer_mono()
  {
  int i;
  int last,val;
  char indx;

  last=0;
  for(i=0;i<BLK_SIZE;i++)
     {
     val=source[i];
     indx=find_index(val-last);
     last+=ampl_table[indx];
     target[i]=indx;
     }
  }
void compress_buffer_stereo()
  {
  int i;
  int last1;
  int last2,val;
  char indx;

  last1=0;
  last2=0;
  for(i=0;i<BLK_SIZE;i++)
     {
     if (i & 1)
        {
        val=source[i];
        indx=find_index(val-last1);
        last1+=ampl_table[indx];
        if (last1>32768 || last1<-32768) abort();
        target[i]=indx;
        }
     else
        {
        val=source[i];
        indx=find_index(val-last2);
        last2+=ampl_table[indx];
        if (last2>32768 || last2<-32768) abort();
        target[i]=indx;
        }
     }
  }

void find_data()
  {
  char w[5]="data";
  int i,d;

  i=0;
  do
     {
     d=fgetc(sf);
     if (d==w[i]) i++;else i=0;
     if (d==EOF) abort();
     }
  while (i<4);
  }


void open_files()
  {
  int i=0;
  sf=fopen(nsource,"rb");
  tf=fopen(ntarget,"wb");
  if (sf==NULL || tf==NULL) abort();
  fseek(sf,0x16,SEEK_SET);
  fread(&chans,1,2,sf);
  fread(&sfreq,1,4,sf);
  find_data();
  fread(&ssize,1,4,sf);
  rsize=ssize;
  blck=rsize/sizeof(source);
  fwrite(&chans,1,2,tf);
  fwrite(&sfreq,1,4,tf);
  fwrite(&ssize,1,4,tf);
  fwrite(&blck,1,4,tf);
  fwrite(&i,1,4,tf);
  fwrite(&i,1,4,tf);
  fwrite(&ampl_table,1,sizeof(ampl_table),tf);
  }

int read_buffer()
  {
  int l;

  if (rsize>sizeof(source)) l=sizeof(source); else l=rsize;
  rsize-=l;
  memset(source,0,sizeof(source));
  return fread(source,1,l,sf);
  }

void write_buffer()
  {
  int i,j;

  i=sizeof(target);
  j=sizeof(source);
  fwrite(&i,1,sizeof(i),tf);
  fwrite(&j,1,sizeof(j),tf);
  fwrite(target,1,sizeof(target),tf);
  }
void timestate(int tim3,int tim4)
  {
  if (tim3<0) tim3=0;
  tim3/=CLOCKS_PER_SEC;
  tim4/=CLOCKS_PER_SEC;
  cprintf("Packing time: %02d:%02d Remainig: %02d:%02d ",tim4/60,tim4%60,tim3/60,tim3%60);
  }

void pack()
  {
  long size;
  int i,j,pt;
  long tim1,tim2,tim3,tim4;

  pt=(ssize/(2*chans))/sfreq;
  cprintf("\n\r"
         "Packing sample : %s --> %s\n\r"
         "Lenght         : %10d KB\n\r"
         "Total blocks   : %10d blocks \n\r"
         "Sample freq.   : %10d Hz\n\r"
         "Sample type    : 16-bit %s\n\r"
         "Aproximation   : %10d \n\r"
         "Playing time   :      %02d:%02d\n\r",
         nsource,ntarget,ssize/1024,blck,sfreq,chans==1?"mono":"stereo",difftype,pt/60,pt%60);
  timestate(88*60+88,88*60+88);
  for (i=0;i<40;i++) putch('.');
  putch('\r');
  tim1=clock();
  do
     {
     size=read_buffer();
     if (chans==1) compress_buffer_mono();
     else compress_buffer_stereo();
     j=ftell(sf)*40/ssize;
     putch('\r');
     tim2=clock();
     tim3=(tim2-tim1);
     tim4=(int)((((float)ssize/ftell(sf))*tim3));
     tim3=tim4-tim3;
     timestate(tim3,tim4);
     for(i=0;i<j;i++) putch(254);
     write_buffer();
     }
  while (rsize);
  cprintf("\n\r"
          "Final lenght   : %10d KB\n\r"
          ,ftell(tf)/1024);
  }

void shutdown()
  {
  fclose(tf);
  fclose(sf);
  }

main(int argc,char **argv)
  {

  cprintf("\nBredy's differencial sample packer for 16-bit samples\n\r"
          "Packing ratio is always 50%%. Packed sample doesn't identical\n\r"
          "with original, but there are very little differences.\n\r"
          "(C)1997 by Bredy.\n\r");
  if (argc<3)
     {
     cprintf("\nUsage: SNDPACK source.wav target.mus [dt]\n\n\r"
             "   dt - type of aproximation. its means:\n\r"
             "         dt = 1: linear (bad quality) \n\r"
             "         dt = 2: xý\n\r"
             "generaly dt = n: xü\n\r"
             "     dt must be in 0 < dt < 6. Default value for dt is 4\n\n\r");
     exit(0);
     }
  nsource=argv[1];
  ntarget=argv[2];
  if (argc==4) sscanf(argv[3],"%d",&difftype);
  Create_table_16();
  open_files();
  pack();
  shutdown();
  }





