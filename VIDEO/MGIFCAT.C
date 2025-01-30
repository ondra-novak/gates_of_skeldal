#include <libs/types.h>
#include <stdio.h>
#include <libs/mgifmem.h>
#include <mem.h>
#include <malloc.h>
#include <conio.h>

MGIF_HEADER_T mhead;
char nohead=1;

char turns[8]="||//--\\\\";
char turncnt;

void show_progress()
  {
  turncnt=(turncnt+1) & 0x7;
  cprintf("> %c <\x8\x8\x8\x8\x8",turns[turncnt]);
  }

void *load_frame(FILE *f,long *size,char *chunks)
  {
  long l;
  int siz;
  void *c;

  if (!fread(&l,4,1,f)) return NULL;
  siz=l>>8;
  *chunks=l & 0xff;
  *size=siz;
  c=malloc(siz);
  if (c==NULL) return NULL;
  if (!fread(c,siz,1,f))
    {
    free(c);
    return NULL;
    }
  return c;
  }

char save_frame(FILE *f,char chunks,void *data,long size)
  {
  long l;

  l=(size<<8)|chunks;
  if (!fwrite(&l,4,1,f)) return 1;
  if (!fwrite(data,size,1,f)) return 1;
  return 0;
  }

void *trans_frame(void *src,int size,char chunks,int soundtrk,long *outlen)
  {
  char *s,*t;
  long l;
  void *tg;

  tg=malloc(size+soundtrk);
  t=tg;s=src;
  if (tg==NULL) return NULL;
  while (chunks--)
    {
    l=*(long *)s;s+=sizeof(l);
    if ((l & 0xff) !=MGIF_SOUND)
      {
      int size=l>>8;
      memcpy(t,&l,sizeof(l));
      t+=4;
      memcpy(t,s,size);
      t+=size;
      s+=size;
      }
    else
      {
      long w;
      w=(soundtrk<<8)|MGIF_SOUND;
      *(long *)t=w;t+=4;
      memset(t,0,soundtrk);
      s+=l>>8;t+=soundtrk;
      }
    }
  *outlen=t-(char *)tg;
  return tg;
  }

void *create_sound(int soundtrk, long *outlen)
  {
  void *tg;
  char *t;

  tg=malloc(soundtrk+4);
  t=tg;
  if (tg==NULL) return NULL;
  *(long *)t=MGIF_SOUND|(soundtrk<<8);
  t+=4;
  memset(t,0,soundtrk);t+=soundtrk;
  *outlen=t-(char *)tg;
  return tg;
  }

#define PRE_SOUND ((256*1024)/2)

int leading_track(FILE *f,int soundtrk)
  {
  int celk,siz;
  void *p;long s;
  char res;
  int frames=0;

  celk=PRE_SOUND;
  while (celk)
    {
    if (celk>soundtrk) siz=soundtrk;else siz=celk;
    p=create_sound(siz,&s);
    celk-=siz;
    res=save_frame(f,1,p,s);frames++;
    free(p);
    if (res) return 0;
    }
  return frames;
  }

char copy_mgif(FILE *tg,FILE *src,int speed)
  {
  MGIF_HEADER_T sh;
  int soundtrk=44100/speed;
  void *p;char chunks;long size;
  void *q;long qsize;
  char skip_lead=0;

  if (!fread(&sh,sizeof(sh),1,src)) return 2;
  if (nohead)
    {
    int res;
    mhead=sh,nohead=0;
    fwrite(&mhead,sizeof(mhead),1,tg);
    res=leading_track(tg,soundtrk);
    if (!res) return 1;
    mhead.frames=res;
    }
  while(sh.frames--)
    {
    p=load_frame(src,&size,&chunks);
    if (p==NULL)return 1;
    if (chunks!=1 || skip_lead)
      {
      q=trans_frame(p,size,chunks,soundtrk,&qsize);
      if (q==NULL) {free(p);return 1;}
      if (save_frame(tg,chunks,q,qsize)) {free(p);free(q);return 1;}
      mhead.frames+=1;
      free(q);
      show_progress();
      skip_lead=1;
      }
    free(p);
    }
  return 0;
  }

char add_mgif(FILE *tg,char *filename,int speed)
  {
  FILE *src;
  char res;

  src=fopen(filename,"rb");
  if (src==NULL) return 255;
  res=copy_mgif(tg,src,speed);
  close(src);
  return res;
  }

int main(int argc,char *argv[])
  {
  int ac;
  int speed;
  FILE *f;

  if (argc<4)
    {
    puts("Malo parametru. Pouziti:\n"
         "\n"
         "MGIFCAT <rychlost> <cil.mgf> <zdroj1.mgf> [zdroj2.mgf [zdroj3.mgf [...]]]\n"
         "\n"
         "Rychlost je ve frames za sekundu\n"
         "POZOR! Cilovy soubor bude prepsan! Neprenasi zvukove stopy!");
    return 1;
    }
  ac=3;
  if (sscanf(argv[1],"%d",&speed)!=1)
    {
    printf("Neplatna rychlost: %s\n",argv[1]);
    return 1;
    }
  f=fopen(argv[2],"wb");
  if (f==NULL)
    {
    printf("Nemohu otevrit vystupni soubor: %s\n",argv[2]);
    return 1;
    }
  nohead=1;
  while (ac<argc)
    {
    cprintf("Pripojuji soubor %s ",argv[ac]);
    switch (add_mgif(f,argv[ac],speed))
      {
      case 1:cputs("Nastala nejaka chyba.\n\r");return 1;
      case 2:cputs("Neplatny MGF soubor.");break;
      case 255:cputs("Soubor nenalezen.");break;
      default: cputs ("..OK..");
      }
    cputs("\n\r");
    ac++;
    }
  fseek(f,0,SEEK_SET);
  fwrite(&mhead,sizeof(mhead),1,f);
  fclose(f);
  return 0;
  }
