#include <stdio.h>
#include <stdlib.h>
#include <graph.h>

#include <conio.h>

#define EVENT_MSG int32_t
#include "globals.h"
#include <memman.c>
#include <strlite.c>
#include <inicfg.c>

TSTR_LIST skini;

#define PLAYERS 3

void *datablast;
int32_t datablastsize;
char *lasterror=NULL;
char relarr[6];
char szBuff[65536];
char sekceid[]="<BLOCK>";

typedef struct s_save
  {
  int viewsector;
  int viewdir;
  int gold;
  short cur_group;
  char autosave;
  char enable_sort;
  char shownames;
  char showlives;
  char zoom_speed;
  char turn_speed;
  char autoattack;
  char music_vol;
  char sample_vol;
  char xbass;
  char bass;
  char treble;
  char stereing;
  char swapchans;
  char out_filter;
  int32_t glob_flags;
  int32_t game_time;
  char runes[5];
  char level_name[12];
  short picks;  //pocet_sebranych predmetu v mysi
  short items_added; //pocet_pridanych predmetu
  int sleep_long;
	int game_flags;
  }S_SAVE;

typedef struct tkouzlo
  {
  word num,um,mge;
  word pc;
  short owner,accnum;     //accnum = akumulacni cislo, owner = kdo kouzlo seslal
  int start;
  short cil;    //kladna cisla jsou postavy zaporna potvory (0 je bez urceni postavy)
  char povaha;
  word backfire; //backfire / 1 = demon , 0 = bez demona
  word wait;   //wait - cekani pocet animaci
  word delay;  //delay - cekani pocet kol
  char traceon;    //jinak noanim - neprehravaji se animace a zvuky
  char spellname[30];
  }TKOUZLO;


typedef struct _tkzlall
  {
  TKOUZLO kouzlo;
  short vlstab[24];
  int32_t flagmap;
  }TKZLALL;

THUMAN postavy[6],postavy2[6];
S_SAVE s;
char mapname[13];
int startsect;
int startsid;


char cz_table_1[]=" 1!3457�908+,-./�+��������\"�?=:_2ABCDEFGHIJKLMNOPQRSTUVWXYZ�\\)6=;abcdefghijklmnopqrstuvwxyz/|(; ";
char cz_table_2[]=" !\"#$%&'()*+,-./0123456789:;<=>?@�BCD�FGH�JK�MN�PQ�ST�VWX�Z[\\]^_`�bcd�fgh�jk�mn�pq�st�vwx�z{|}~ ";
char cz_table_3[]=" !\"#$%&'()*+,-./0123456789:;<=>?@AB���FGHIJK�M��PQ����VWXY�[\\]^_`ab���fghijk�m��pq����vwxy�{|}~ ";
char *cz_key_tabs[]={cz_table_1,cz_table_2,cz_table_3};

word keyconv(word key)
  {
  int i;
  static char cz_mode=0;
  char c,d;

  i=key;
  d=i>>8;
  c=i & 0xff;
  if (c=='+' && d<55 && !cz_mode) cz_mode=2;
  else if (c=='=' && d<55 && !cz_mode) cz_mode=1;
  else if (c>32 && c<127 && d<=53)
              {
              c=cz_key_tabs[cz_mode][c-32];
              i=d;
              i=(i<<8)+c;
              cz_mode=0;
              return i;
              }
     else
       return i;

   return 0;
  }


#define ZAKLAD_CRC 0xC005

static word vypocet_crc(char *data,int32_t delka)
  {
  uint32_t l=0;
  do
     {
     l=(l<<8)|(delka>0?*data++:0);delka--;
     l=(l<<8)|(delka>0?*data++:0);delka--;
     l%=ZAKLAD_CRC;
     }
  while(delka>-1);
  return l & 0xffff;
  }

static simple_cz_input(char *buff,int maxchars)
  {
  int cnt=strlen(buff);
  int w;
  char *a;
  a=alloca(cnt+1);
  strcpy(a,buff);
  maxchars--;
  cputs(buff);
  do
    {
    w=_bios_keybrd(_KEYBRD_READ);
    w=keyconv(w);
    if (w==0) continue;
    switch (w & 0xff)
      {
      case 13: return;
      case 0x8: if (cnt>0)
                  {
                  cnt--;buff[cnt]=0;cputs("\x8 \x8");
                  }
                break;
      case 0:break;
      case 27:strcpy(buff,a);
              return;
      default: if (cnt<maxchars) putch(buff[cnt++]=w & 0xff);
      }
    }
  while(1);
  }

void *addmem(void *to,void *from,int size)
  {
  memcpy(to,from,size);
  return (void *)((char *)to+size);
  }

void *loadmem(void *to,void *from,int size)
  {
  memcpy(to,from,size);
  return (void *)((char *)from+size);
  }

int32_t load_section(FILE *f,void **section, int *sct_type,int32_t *sect_size)
//
  {
  int32_t s;
  char c[20];

  *section=NULL;
  fread(c,1,sizeof(sekceid),f);
  if (strcmp(c,sekceid)) return -1;
  fread(sct_type,1,sizeof(*sct_type),f);
  fread(sect_size,1,sizeof(*sect_size),f);
  fread(&s,1,sizeof(s),f);
  *section=malloc(*sect_size);
  s=fread(*section,1,*sect_size,f);
  return s;
  }


static void load_specific_file(char *slotname,char *filename,void **out,int32_t *size)
  {
  FILE *slot;
  char *c,*d;
  int32_t siz;
  char fname[12];
  char succes=0;

  slot=fopen_icase(slotname,"rb");
  if (slot==NULL)
     {
     *out=NULL;
     return;
     }
  fseek(slot,SAVE_NAME_SIZE,SEEK_CUR);
  fread(fname,1,12,slot);
  while(fname[0] && !succes)
     {
     fread(&siz,1,4,slot);
     if (!strncmp(fname,filename,12)) succes=1; else
           {
           fseek(slot,siz,SEEK_CUR);
           fread(fname,1,12,slot);
           }
     }
  if (succes)
     {
     *out=malloc(siz);
     fread(*out,1,siz,slot);
     *size=siz;
     }
  else *out=NULL;
  fclose(slot);
  }

void return_spells(int poc,TKZLALL *kzl)
  {
  int i;

  for(i=0;i<poc;i++,kzl++)
    {
    int j;
    THUMAN *h=postavy+kzl->kouzlo.cil-1;

    if (kzl->kouzlo.cil>0)
      {
      for (j=0;j<22;j++) h->stare_vls[j]+=kzl->vlstab[j];
      h->stare_vls[VLS_KOUZLA]&=~(kzl->flagmap & 0xffff);
      }
    }
  }

void zero_inv(THUMAN *h)
  {
  h->inv_size=6;
  memset(h->inv,0,sizeof(h->inv));
  memset(h->wearing,0,sizeof(h->wearing));
  memset(h->prsteny,0,sizeof(h->prsteny));
  memcpy(h->vlastnosti,h->stare_vls,sizeof(h->vlastnosti));
  h->demon_save=NULL;
  h->sektor=startsect;
  h->direction=startsid;
  h->groupnum=1;
  }


void zero_all_inv()
  {
  THUMAN *h=postavy;int i;

  for (i=0;i<6;i++,h++) if (h->used) zero_inv(h);
  }

void unpack_save(void *in)
  {
  int kouzel;
  TKZLALL *kzl;
  int i;

  in=loadmem(&s,in,sizeof(s)); //load basic info
  in=(void *)((char *)in+s.picks*2+s.items_added*sizeof(TITEM)); //skip items;
  in=loadmem(&kouzel,in,sizeof(kouzel));  //load spell table
  kzl=in;                             //signup spell table for future use
  in=(void *)((char *)in+sizeof(TKZLALL)*kouzel);
  in=loadmem(postavy,in,sizeof(postavy)); //load character table
  for (i=0;i<6;i++)
    if (postavy[i].demon_save!=NULL)  //correct demons;
      in=loadmem(postavy+i,in,sizeof(THUMAN));
  //skip dialog info.
  //load done;
  return_spells(kouzel,kzl);
  zero_all_inv();
  s.viewsector=startsect;
  s.viewdir=startsid;
  s.picks=0;
  s.items_added=0;
  s.glob_flags=0;
  s.game_time=0;
  strcopy_n(s.level_name,mapname,12);
  memset(s.runes,0,sizeof(s.runes));
  }

void del_character(char c)
  {
  int i;
  for(i=c;i<5;i++) relarr[i]=relarr[i+1];
  relarr[i]=0;
  }

void add_character(char c)
  {
  THUMAN *h=postavy+c;
  int i;

  if (!h->used)
    {
    lasterror="Musis vybrat postavu!";return;
    }
  for (i=0;i<6;i++) if (relarr[i]==c+1)
    {
    del_character(i);return;
    }
  for (i=0;i<6;i++) if (!relarr[i]) break;
  if (i==6)
    {
    lasterror="Nevim sice jak se ti to povedlo, ale neni uz misto!";return;
    }
  relarr[i]=c+1;
  }

char nvlast[][16]=
  {"Sila","Schopnost magie","Pohyblivost","Obratnost","Max zraneni",
  "Kondice","Max mana","Obrana(dolni)","Obrana(Horni)","Utok(Dolni)",
  "Utok(Horni)","Ohen","Voda","Zeme","Vzduch","Mysl","Zivoty Regen",
  "Mana Regen","Kondice Regen","Magicka sila(D)", "Magicka sila(H)","","Ucinnek zasahu","*"};

char *zbrane[]={"Mec","Sekera","Kladivo","Hul","Dyka","Strelne","Specialni"};


char build_players()
  {
  int i;
  char z=0;

  memset(postavy2,0,sizeof(postavy2));
  for (i=0;i<6;i++)
    if (relarr[i])postavy2[i]=postavy[relarr[i]-1],z=1;
  return z;
  }

void c_info(int b)
  {
  THUMAN *h=postavy+b;
  int i;

  do
  {
  _clearscreen(_GCLEARSCREEN);
  for(i=0;i<22;i++)
    {
    cprintf("%-17s %3d    ",nvlast[i],h->stare_vls[i]);
    if (i<7)
     cprintf("%-12s %3d    ",zbrane[i],h->bonus_zbrani[i]);
    if (i<7)
      switch (i)
      {
      case 0: cprintf("Jmeno:     %s",h->jmeno);break;
      case 1: cprintf("Uroven:    %d",h->level);break;
      case 2: cprintf("Zkusenost: %d",h->exp);break;
      case 3: cprintf("Pohlavi:   %s",h->female?"Zena":"Muz");break;
      case 4: cprintf("Portret:   XICHT%02X.PCX",h->xicht);break;
      case 5: cprintf("Jidlo:     %d",h->jidlo/360);break;
      case 6: cprintf("Voda:      %d",h->voda/360);break;
      }
    cprintf("\n\r");
    }
  cprintf("ENTER - zmena jmena, P - pohlavi, X - vzhledu, ESC - navrat:");
  do
    {i=toupper(getche());putch(8);}
  while (i!=13 && i!='X' && i!=27 && i!='P');
  if (i==13)
    {
    cprintf("\r\n\n Zmen jmeno:");simple_cz_input(h->jmeno,sizeof(h->jmeno));
    }
  else if (i=='X')
    {
    cprintf("\r\n\n Jelikoz jednoduchost tohoto programu neumoznuje prohizet si obrazky je nutne\n\r"
            "tuto volbu odzkouset metodou pokus omyl. Nikdo totiz nezaruci, zda v novem \n\r"
            "dobrodruzstvi se nektere z postav nezmeni pohlavi (na obrazku).\r\n"
            "V puvodnim SKELDALU:\n\r"
            "--------------------\n\r"
            "Muzi: 0, 2, 3, 4, 8(Roland), 12(Gralt), 13(Erik)\n\r"
            "Zeny: 1, 5, 6, 7\n\r"
            "\nZmenit vzhled <%d>, 'x' storno:",h->xicht);
    if (scanf("%hd",&h->xicht)==0) while (getchar()!='\n');
    }
  else if (i=='P') h->female=!h->female;
  }
  while (i!=27) ;
  }


char assign_players()
  {
  int i;
  char c;
  char backrel[6];

  do
    {
    _clearscreen(_GCLEARSCREEN);
    if (lasterror!=NULL) cprintf("%s\x7\r\n\n",lasterror);
    lasterror=NULL;
    memset(backrel,0,sizeof(backrel));
    for (i=0;i<6;i++) if (relarr[i]) backrel[relarr[i]-1]=i+1;
    cprintf("%-33s%s\n\r====================================================\n\r","Stare dobrodruzstvi","Nove dobrodruzstvi");
    for(i=0;i<6;i++)
      {
      if (postavy[i].used)
        cprintf("%c%d.%-30s",backrel[i]?'*':' ',i+1,postavy[i].jmeno);
      else
        cprintf(" %d.%-30s",i+1,"<prazdny slot>");
      if (relarr[i])
        cprintf("%c.%-30s",i+'a',postavy[relarr[i]-1].jmeno);
      else
        cprintf("%c.%-30s",i+'a',"<prazdny slot>");
      cputs("\n\r");
      }
    cputs("\n\r");
    cputs("[1]-[6]         prenese postavu,\n\r"
          "[a]-[f]         maze postavu.\n\r"
          "[SHIFT]+[1]-[6] info o postave\n\r"
          "[S]             ulozit\n\r"
          "[Q] nebo [ESC]  konec bez ulozeni\n\n\r");
    cprintf("Doporuceny pocet postav v novem dobrodruzstvi je %d.\n\r",PLAYERS);
    putch('>');
    i=_bios_keybrd(_KEYBRD_READ);
    c=toupper(i & 0xff);i>>=8;
    switch (c)
      {
      case 27:
      case 'Q':return 1;
      case 'S':return 0;
      default:
        if (i==0) lasterror="Prepni na anglickou klavesnici!!!";
        if (i>=2 && i<=7)
           if (*(char *)0x417 & 0x3) c_info(i-2);
           else add_character(i-2);
        if (c>='A' && c<='F') del_character(c-'A');
        break;
      }
    }
  while(1);
  }

char *scan_saves(char *text,char *path,char mustexists)
  {
  int i;
  FILE *f;

  do
    {
    _clearscreen(_GCLEARSCREEN);
    cprintf("%s\n\r============================\n\r",text);
    if (lasterror!=NULL) cprintf("%s\x7\r\n\n",lasterror);
    lasterror=NULL;
    for(i=0;i<10;i++)
      {
      sprintf(szBuff,"%sSLOT%02d.SAV",path,i);
      cprintf("%d. ",(i+1)%10);
      f=fopen_icase(szBuff,"rb");
      if (f!=NULL)
        {
        fread(szBuff,1,34,f);szBuff[34]=0;
        fclose(f);
        cprintf("%s\r\n",szBuff);
        }
      else
        cputs("<nic>\r\n");
      }
    cputs("\nVyber [1]-[9] a [0] pozici.\n\r[ESC] zrusit.\n\r");
    do
      i=_bios_keybrd(_KEYBRD_READ)>>8;
    while (i>11);
    if (i==1) return NULL;
    if (i==0) lasterror="Prepni na anglickou klavesnici!!!";
      else
      {
      i-=2;
      sprintf(szBuff,"%sSLOT%02d.SAV",path,i);
      if (!mustexists || access(szBuff,F_OK)==0) return szBuff;
      }
    }
  while(1);
  }

int tracemap(char *name)
  {
  FILE *f;
  void *section;
  int type;
  int32_t  size,s;

  f=fopen_icase(name,"rb");
  if (f==NULL) return -1;
  do
    {
    s=load_section(f,&section,&type,&size);
    if (s!=size)
      {
      free(section);
      fclose(f);
      return -1;
      }
    if (type==A_MAPGLOB)
      {
      MAPGLOBAL mglob;
      memcpy(&mglob,section,sizeof(mglob));
      startsect=mglob.start_sector;
      startsid=mglob.direction;
      }
    free(section);
    }
  while (type!=A_MAPEND);
  fclose(f);
  return 0;
  }

void *build_gametmp(int *size)
  {
  int siz=*size;
  void *zac,*p;
  int i,crc;

  siz+=256;
  p=zac=malloc(siz);
  p=addmem(p,&s,sizeof(s)); //add game info
  //skip picks
  //skip items added
  i=0;//no spells;
  p=addmem(p,&i,4);
  p=addmem(p,postavy2,sizeof(postavy2));
  p=addmem(p,&i,4); //no dialogs...
  *size=(char *)p-(char *)zac;
  crc=vypocet_crc(zac,*size);
  p=addmem(p,&crc,2);
  size[0]+=2;
  return zac;
  }

void save_savegame(char *soubor)
  {
  FILE *f;
  void *p;
  int s;

  f=fopen_icase(soubor,"wb");
  strcpy(szBuff,"TRANSAV");
  fwrite(szBuff,SAVE_NAME_SIZE,1,f);
  strcpy(szBuff,"_GAME.TMP");
  fwrite(szBuff,12,1,f);
  s=datablastsize;
  p=build_gametmp(&s);
  fwrite(&s,4,1,f);
  fwrite(p,s,1,f);
  free(p);
  szBuff[0]=0;
  fwrite(szBuff,12,1,f);
  fclose(f);
  }

void begin_program(char *sourceadv,char *targetadv)
  {
  TSTR_LIST src,trg;
  char *c;

  src=read_config(sourceadv);
  if (src==NULL)
    {
    cprintf("Nemohu otevrit zdrojove dobrodruzstvi: %s\n\r",sourceadv);
    exit(1);
    }
  trg=read_config(targetadv);
  if (trg==NULL)
    {
    cprintf("Nemohu otevrit cilove dobrodruzstvi: %s\n\r",targetadv);
    exit(1);
    }
  c=get_text_field(trg,"DEFAULT_MAP");
  if (c==NULL)
    {
    cprintf("Vystupni dobrodruzstvi %s musi obsahovat polozku DEFAULT_MAP\n\r", targetadv);
    exit(1);
    }
  strcopy_n(mapname,c,12);mapname[12]=0;
  c=get_text_field(trg,"CESTA_MAPY");
  if (c==NULL)
    {
    cprintf("Vystupni dobrodruzstvi %s musi obsahovat polozku CESTA_MAPY\n\r", targetadv);
    exit(1);
    }
  sprintf(szBuff,"%s%s",c,mapname);
  if (tracemap(szBuff))
    {
    cprintf("Nemohu precist mapu %s\n\r",szBuff);
    exit(1);
    }
  c=get_text_field(trg,"CESTA_POZICE");
  if (c==NULL)
    {
    cprintf("Vystupni dobrodruzstvi %s musi obsahovat polozku CESTA_POZICE\n\r", targetadv);
    exit(1);
    }
  c=get_text_field(src,"CESTA_POZICE");
  if (c==NULL)
    {
    cprintf("Vstupni dobrodruzstvi %s musi obsahovat polozku CESTA_POZICE\n\r", sourceadv);
    exit(1);
    }
  c=scan_saves("Vyber pozici:",c,1);
  if (c==NULL) exit(0);
  load_specific_file(c,"_GAME.TMP",&datablast,&datablastsize);
  if (datablast==NULL)
    {
    cprintf("Soubor %s: Pristup odmitnut - pozice je pozkozena!\n\r",c);
    exit(1);
    }
  unpack_save(datablast);
  do
    {
    do
      if (assign_players()) exit(1);
    while (!build_players());
    zpet:
    c=get_text_field(trg,"CESTA_POZICE");
    c=scan_saves("Ulozit na pozici:",c,0);
    }
  while (c==NULL);
  if (access(c,F_OK)==0)
      {
      cprintf("Prejes si prepsat existujici pozici? (A/cokoliv):");
      if (toupper(getche())!='A') goto zpet;
      }
  save_savegame(c);
  }

static char help1[]=
  "Tento program prenasi ulozene pozice mezi jednotlivymi dobrodruzstvimi.\n\r"
  "Jelikoz ulozena pozice je znacne zavisla na mapach a definicich, je mozne\n\r"
  "timto programem prenaset pouze charakteristiky postav. Navic se mohou \n\r"
  "objevit nejake komplikace vznikle s nemoznosti predvidat komplexnost obou\n\r"
  "dobrodruzstvi.\n\r\n"
  "Program ma dva zapisy parametru:\n\r"
  "================================\n\r\n"
  "TRANSAV <cil.adv>\n\r\n"
  "     Prenasi puvodni pozici ze hry Brany Skeldalu do noveho dobrodruzstvi\n\r\n"
  "TRANSAV <zdroj.adv> <cil.adv>\n\r\n"
  "     Prenasi pozici mezi dvema dobrodruzstvi\n\r\n";
static char help2[]=
  "Jak uz bylo receno, prenasi se pouze charakteristiky postav. Neprenasi se\n\r"
  "predmety a runy. Nova pozice se jmenuje TRANSAV, pak si ji prejmenujte. \n\r"
  "Ihned po nahrati prenesene pozice okamzite tuto pozici opet ulozte, aby SKELDAL\n\r"
  "doplnil chybejici informace vztahujici se k novemu dobrodruzstvi. \n\r"
  "V pokrocilejsich castech hry mohou tyto chybejici informace zpusobovat nejake\n\r"
  "komplikace...\n\r\n"
  "Prenasec pozic pro Vas napsal Ondrej Novak, programator hry Brany Skeldalu.\n\r\n"
  "PS: Deaktivujte ovladace ceske klavesnice!!!\n\r";

void main(int argc,char **argv)
  {
  char szbuff[20];

  if (argc!=2 && argc!=3)
    {
    cputs(help1);
    cputs("--- klavesu ---");getche();putch('\r');
    cputs(help2);
    return;
    }
  memset(relarr,0,sizeof(relarr));
  if (argc==2)
    begin_program("SKELDAL.INI",argv[1]);
  else
    begin_program(argv[1],argv[2]);
  }





