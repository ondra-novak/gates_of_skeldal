#include <libs/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <mem.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <wav.c>

#define BLK_SIZE 100000
#define CHANNELS 256
#define SND_NAME ".SNF"

typedef struct volstruc
  {
  int volslow,volsmax;
  int volume;
  int volume_end;
  int vls;

  }TVOLSTRUC;

typedef struct tchaninfo
  {
  FILE *snd;
  char chans;
  char bit8;
  long repstart;
  long id ;
  long size;
  TVOLSTRUC left,right;
  }TCHANINFO;

TCHANINFO chaninfo[CHANNELS];

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

short ampl_table[256];
signed short source[BLK_SIZE];
char target[BLK_SIZE];
int glob_volume=256;

long sfreq;
unsigned short chans;
FILE *sf;
int tf;

char *nsource;
char *ntarget;
long ssize;
long rsize,blck;
char difftype=4;

int last_channel;

long next_frame;

int find_free_channel()
  {
  int i;
  for(i=0;i<256;i++) if (chaninfo[i].snd==NULL) return i;
  printf("Nemohu pridat zvuk. Jiz je zaplneno vsech %d kanal�\n",CHANNELS);
  return -1;
  }

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

void compress_buffer_stereo(int size)
  {
  int i;
  static int last1=0;
  static int last2=0;
  int val;
  char indx;


  for(i=0;i<size;i++)
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

void open_wav(char *wav)
  {
  TCHANINFO *inf;
  int i;
  struct t_wave *wavinf;
  FILE *snd;

  i=find_free_channel();
  if (i==-1) return;
  last_channel=i;
  inf=chaninfo+i;
  snd=fopen(wav,"rb");
  if (snd==NULL)
     {
     printf("Soubor %s neexistuje.\n",wav);
     return;
     }
  if (find_chunk(snd,WAV_FMT)==-1)
     {
     printf("Soubor %s ma poskozenou hlavicku\n",wav);
     fclose(snd);
     return;
     }
  wavinf=(struct t_wave *)malloc(get_chunk_size(snd));
  if (wavinf==NULL)
     {
     puts("Nedostatek pam�ti.");
     return;
     }
  read_chunk(snd,wavinf);
  if (wavinf->wav_mode!=1)
     {
     printf("Tento program podporuje pouze WAVy typu 1 (%s)\n",wav);
     free(wavinf);
     fclose(snd);
     return;
     }
  inf->chans=wavinf->chans;
  inf->bit8=wavinf->freq*wavinf->chans==wavinf->bps;
  inf->repstart=-1;
  inf->id=-1;
  inf->left.volume=32768;
  inf->right.volume=32768;
  inf->left.vls=0;
  inf->right.vls=0;
  free(wavinf);
  if (find_chunk(snd,WAV_DATA)==0)
     {
     printf("Soubor %s je poskozen\n",wav);
     fclose(snd);
     return;
     }
  inf->size=get_chunk_size(snd);
  fseek(snd,4,SEEK_CUR);
  inf->snd=snd;
  }

void calc_vls(TVOLSTRUC *vls)
  {
  vls->volslow--;
  if (vls->volslow<=0)
     {
     vls->volslow+=vls->volsmax;
     vls->volume+=vls->vls;
     if ((vls->vls>0 && vls->volume>=vls->volume_end)||
        (vls->vls<0 && vls->volume<=vls->volume_end))
        {
          vls->volume=vls->volume_end;
          vls->vls=0;
        }
     }
  }

void mix_buffer(int size)
  {
  int chan,i;
  memset(source,0,size*2);
  for(chan=0;chan<CHANNELS;chan++)
     {
     if (chaninfo[chan].snd!=NULL)
        {
        TCHANINFO *inf=chaninfo+chan;

        for(i=0;i<size;i+=2)
           {
           int left,right;
           if (inf->bit8)
              {
              fread(&left,1,1,inf->snd);left=(short)(left*256);
              left^=0xffff8000;
              inf->size--;
              }
           else
              {
              fread(&left,1,2,inf->snd);left=(short)left;
              inf->size-=2;
              }
           if (inf->chans==1) right=left;
           else
              if (inf->bit8)
                 {
                 fread(&right,1,1,inf->snd);right=(short)(right*256);
                 right^=0xffff8000;
                 inf->size--;
                 }
              else
                 {
                 fread(&right,1,2,inf->snd);right=(short)right;
                 inf->size-=2;
                 }
           left=(int)left*inf->left.volume/(32768);
           right=(int)right*inf->right.volume/(32768);
           left=left*glob_volume/256;
           right=right*glob_volume/256;
           calc_vls(&inf->left);
           calc_vls(&inf->right);
           left+=source[i];
           right+=source[i+1];
           if (left>32767) left=32767;
           if (left<-32767) left=-32767;
           if (right>32767) right=32767;
           if (right<-32767) right=-32767;
           source[i]=left;
           source[i+1]=right;
           if (inf->size<=0)
              {
              if (inf->repstart!=-1)
                 {
                 fseek(inf->snd,0,SEEK_SET);
                 find_chunk(inf->snd,WAV_DATA);
                 inf->size=get_chunk_size(inf->snd);
                 fseek(inf->snd,4,SEEK_CUR);
                 fseek(inf->snd,inf->repstart,SEEK_CUR);
                 inf->size-=inf->repstart;
                 }
              else
                 {
                 fclose(inf->snd);
                 inf->snd=NULL;
                 break;
                 }
              }
           }
        }
     }
  }


void open_files(char *ntarget)
  {
  tf=open(ntarget,O_BINARY | O_RDWR);
  if (!tf) abort();
  lseek(tf,0,SEEK_SET);
  read(tf,&gh,sizeof(gh));
  memcpy(gh.ampl_table,ampl_table,sizeof(ampl_table));
  gh.snd_freq=22050;
  }


void read_frame()
  {
  long x;
  read(tf,&x,4);
  x>>=8;
  next_frame=tell(tf)+x;
  }

long find_sound_action()
  {
  long x;char c;

  do
     {
     read(tf,&x,4);
     c=x;x>>=8;
     if (c==4) return x;
     lseek(tf,x,SEEK_CUR);
     }
  while (1);
  }


void press_sound_chunk()
  {
  int siz;


  siz=find_sound_action();
  mix_buffer(siz);
  compress_buffer_stereo(siz);
  write(tf,&target,siz);
  }

void close_files()
  {
  int i;
  lseek(tf,0,SEEK_SET);
  write(tf,&gh,sizeof(gh));
  fclose(sf);
  close(tf);
  for(i=0;i<CHANNELS;i++)
     if (chaninfo[i].snd!=NULL)
        {
        fclose(chaninfo[i].snd);
        chaninfo[i].snd=NULL;
        }
  }


void ozvuceni()
  {
  int i;

  for(i=1;i<gh.frames;i++)
     {
     cprintf("frame %d\r",i);
     read_frame();
     press_sound_chunk();
     lseek(tf,next_frame,SEEK_SET);
     }
  }

#define get_num(c,n) {sscanf(c,"%d",&n);while (*c==32) c++;c=strchr(c,32);}

int find_channel_id(int id)
  {
  int i;
  for(i=0;i<256;i++) if (chaninfo[i].id==id) return i;
  printf("Nemohu najit kanal s ID: %d\n",id);
  return -1;
  }

void interpret_line(FILE *f)
  {
  char line_buf[1024];
  char *c=line_buf;
  TVOLSTRUC *v;
  TCHANINFO *inf;
  int num;char fcid=0;

  fgets(line_buf,1024,f);
  while (c!=NULL)
     {
     while (*c==32) c++;
     if (!*c) break;
     if (*c=='@')
        {
        char command[24];

        c++;
        sscanf(c,"%23s",command);c=strchr(c,32);
        strupper(command);
        if (!strcmp(command,"ID"))
           {
           get_num(c,num);
           if (fcid) chaninfo[last_channel].id=num;
           fcid=0;
           if ((last_channel=find_channel_id(num))==-1) return;
           inf=chaninfo+last_channel;
           }
        else if (!strcmp(command,"R")) v=&inf->left;
        else if (!strcmp(command,"L")) v=&inf->right;
        else if (!strcmp(command,"VOL"))
           {
           get_num(c,num);
           v->volume=num*128;
           }
        else if (!strcmp(command,"VOLEND"))
           {
           get_num(c,num);
           v->volume_end=num*128;
           v->vls=0;
           }
        else if (!strcmp(command,"SPEED"))
           {
           get_num(c,v->vls);v->volsmax=1;v->volslow=1;
           if (v->volume>v->volume_end) v->vls=-v->vls;
           }
        else if (!strcmp(command,"SLOW"))
           {
           get_num(c,v->volsmax);v->vls=1;v->volslow=1;
           if (v->volume>v->volume_end) v->vls=-v->vls;
           }
        else if (!strcmp(command,"REPSTART")){ get_num(c,inf->repstart);}
        else if (!strcmp(command,"GLOBVOL")) {get_num(c,glob_volume);}
        }
     else
        {
        char filename[128];
        sscanf(c,"%127s",filename);
        c=strchr(c,32);
        open_wav(filename);
        inf=chaninfo+last_channel;
        fcid=1;
        }
     }
  }

void call_script(char *script_name)
  {
  FILE *scr,*snf;
  char name[256];
  char snd_name[256],*c;
  int i,fr,lfr=0,wfr=-1;

  scr=fopen(script_name,"r");
  strcpy(snd_name,script_name);
  c=strrchr(snd_name,'.');
  if (c==NULL) strcat(snd_name,SND_NAME);
  else strcpy(c,SND_NAME);
  if (scr==NULL)
     {
     printf("Nemohu otev��t script: %s\n",script_name);
     exit(1);
     }
  snf=fopen(snd_name,"r");
  i=fscanf(scr,"%255s",name);
  open_files(name);
  if (i!=1)
     {
     printf("Chyba ve script souboru: %s. Prvni mus� b�t jm�no c�lov�ho souboru.\n",scr);
     exit(1);
     }
  while ((i=fgetc(scr))!=EOF && i!='\n');
  for(fr=1;fr<gh.frames;fr++)
     {
     cprintf("frame %d\r",fr);
     read_frame();
     do
        {
        if (wfr==-1)
          {
          i=fgetc(scr);
          if (i!=EOF)
             {
             if (i!='#')
                {
                fscanf(snf,"%d",&lfr);
                while ((i=fgetc(scr))!=EOF && i!='\n');
                }
             else
               {
               fscanf(scr,"%d",&wfr);
               wfr+=lfr-1;
               }
             }
          else wfr=9999999;
          }
       if (wfr<=fr && wfr!=-1)
           {
           interpret_line(scr);
           wfr=-1;
           }
        }
     while (wfr==-1);
     press_sound_chunk();
     lseek(tf,next_frame,SEEK_SET);
     }
  fclose(scr);
  if (snf!=NULL) fclose(snf);
  }

main(int argc,char *argv[])
  {

  if (argc<2)
     {
     puts("\nPouziti:  MGFSOUND film.mgf zvuk.wav [i]");
     puts("\nnebo:     MGFSOUND script.scr");
     puts("\nKde _i_ je komprimacni krivka (viz SNDPACK) (default:4)");
     exit(0);
     }
  if (argc>3)
     {
     sscanf(argv[3],"%d",&difftype);
     }
  Create_table_16();
  if (argc==2)
     call_script(argv[1]);
  else
     {
     open_wav(argv[2]);
     open_files(argv[1]);
     ozvuceni();
     }
  close_files();
  }

