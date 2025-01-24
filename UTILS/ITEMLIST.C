#include <types.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#define SV_ITLIST 0x8001
#define SV_SNDLIST 0x8002
#define SV_END    0x8000


typedef struct titem
  {
  char jmeno[32];   //32        Jmeno predmetu
  char popis[32];   //64
  short zmeny[24];  //112       Tabulka, jakych zmen ma na hracovy vlastnosti
  short podminky[4];//120       Tabulka, jake vlastnosti musi mit hrac k pouziti predmetu
  short hmotnost,nosnost,druh; //126  druh = Typ predmetu
  short umisteni;              //128  Kam se predmet umisti?
  word flags;                  //130  ruzne vlajky
  short spell,magie,sound_handle;//136  specialni kouzla / rukojet zvuku
  short use_event;             //140  specialni udalost
  unsigned short ikona,vzhled; //144  ikony a vzhled
  short user_value;            //146  uzivatelska hodnota
  short keynum;                //148 cislo klice
  short polohy[2][2];          //156 souradnice poloh pro zobrazeni v inv
  char typ_zbrane;              //160 Typ zbrane
  char unused;
  short sound;                  //cislo zvuku
  short v_letu[16];             //192
  int cena;
  char weapon_attack;           //relativni handle k souboru s animaci utok
  char hitpos;                  //pozice zasahu animace
  short rezerva[13];           //224 rezervovane
  }TITEM;

#define isweapon(p) ((p)>=1 && (p)<=3)

char sekceid[]="<BLOCK>";
TITEM *itlist;
int item_count;
char *sound;
char listsize;
int pagelen;

char pr_name=0,pr_type=0,pr_sound=0,pr_umist=0,pr_zbran=0,pr_hmotn=0,pr_popis=0;
char grp_sort=0,pr_number=0,pr_cena=0;

char _typy_zbrani[]=
  "Me‡\0"
  "Sekera\0"
  "Kladivo\0"
  "H–l\0"
  "D˜ka\0"
  "St©eln \0"
  "Ostatn¡\0";

char _typy_veci[]=
  "Nespecifikov no\0"
  "Zbra¤ tv ©¡ v tv ©\0"
  "Vrhac¡ zbra¤\0"
  "St©eln  zbra¤\0"
  "Zbroj\0"
  "Svitek / H–lka\0"
  "Lektvar\0"
  "Voda\0"
  "J¡dlo\0"
  "Speci ln¡\0"
  "Runa\0"
  "Pen¡ze\0"
  "Svitek s textem\0"
  "Prach\0"
  "Ostatn¡\0";

char _umisteni_veci[]=
  "Nikam\0"
  "Zavazadlo\0"
  "Na tˆlo (naho©e)\0"
  "Na tˆlo (dole)\0"
  "Na hlavu\0"
  "Na nohy\0"
  "Kutna\0"
  "Na krk\0"
  "Do ruky\0"
  "Obouru‡\0"
  "Prsten\0"
  "›¡p\0";


long load_section(FILE *f,void **section, int *sct_type,long *sect_size)
//
  {
  long s;
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

void load_items_dat(char *name)
  {
  FILE *f;
  void *temp;
  int type;
  long size;

  f=fopen(name,"rb");
  if (f==NULL)
     {
     printf("Nemohu otevrit soubor %s\n",name);
     exit(1);
     }
  do
     {
     load_section(f,&temp,&type,&size);
     switch (type)
        {
        case SV_ITLIST:itlist=temp;item_count=size/sizeof(TITEM);break;
        case SV_SNDLIST:sound=temp;listsize=size;break;

        case SV_END:
        default: free(temp);break;
        }
     }
  while (type!=SV_END);
  fclose(f);
  }

char *find_str(char *sound,int number,long listsize)
  {
  char *c;
  static char none[]="<nic>";
  static char unknown[]="<ref.error>";

  c=sound;
  if (!number) return none;
  number--;
  while (c-sound<listsize && number--) c+=strlen(c)+1;
  if (c-sound<listsize) return c;
  return &unknown;
  }

void print_polozka(int num,TITEM *t)
  {
  if (pr_number) printf("%3d³",num);
  if (pr_name) printf("%-32s³",t->jmeno);
  if (pr_type) printf("%-20s³",find_str(_typy_veci,t->druh+1,sizeof(_typy_veci)));
  if (pr_umist) printf("%-20s³",find_str(_umisteni_veci,t->umisteni+1,sizeof(_umisteni_veci)));
  if (pr_zbran) printf("%-10s³",isweapon(t->druh)?find_str(_typy_zbrani,t->typ_zbrane+1,sizeof(_typy_zbrani)):" ");
  if (pr_hmotn) printf("%6d³",t->hmotnost);
  if (pr_cena) printf("%6d³",t->cena);
  if (pr_sound) printf("%-12s³",find_str(sound,t->sound,listsize));
  if (pr_popis) printf("%s",t->popis);
  puts("");
  }

void print_head()
  {
  int lines=0;
  if (pr_number) printf("ID  "),lines+=4;
  if (pr_name) printf("%-32s ","Jmeno veci"),lines+=33;
  if (pr_type) printf("%-20s ","Typ veci"),lines+=21;
  if (pr_umist) printf("%-20s ","Umisteni"),lines+=21;
  if (pr_zbran) printf("%-10s ","Zbran"),lines+=11;
  if (pr_hmotn) printf("%-6s ","Vaha"),lines+=7;
  if (pr_cena) printf("%-6s ","Cena"),lines+=7;
  if (pr_sound) printf("%-12s ","Zvuk"),lines+=13;
  if (pr_popis) printf("%s","Popis"),lines+=6;
  puts("");
  if (lines) while (lines--) putc(196,stdout);
  puts("");
  }

void endpage(int i)
  {
  if (pagelen && i%pagelen==0)
     {
     if (i) putc(12,stdout);
     print_head();
     }
  }

void counting_sort(TITEM *sr,TITEM *tg,int count,int *idnums)
  {
  int tridy[30][20];
  int i,j,c;
  TITEM *s;

  memset(tridy,0,sizeof(tridy));
  for(i=0,s=sr;i<count;i++,s++) tridy[s->druh][isweapon(s->druh)?s->typ_zbrane:0]++;
  c=0;
  for(i=0;i<30;i++)
     for(j=0;j<20;j++)
        {
        register int d;
        d=tridy[i][j];
        tridy[i][j]=c;
        c+=d;
        }
  for(i=0,s=sr;i<count;i++,s++)
     {
     int c=tridy[s->druh][isweapon(s->druh)?s->typ_zbrane:0]++;
     tg[c]=*s;
     idnums[c]=i;
     }
  }

void print_list()
  {
  int i;
  TITEM *t;

  if (!pagelen) print_head();
  if (!grp_sort)
     {
     for(i=0,t=itlist;i<item_count;i++,t++)
        {
        endpage(i);
        print_polozka(i,t);
        }
     }
  else
     {
     TITEM *list;
     int *nums;
     list=(TITEM *)malloc(sizeof(TITEM)*item_count);
     nums=(int *)malloc(sizeof(int)*item_count);
     counting_sort(itlist,list,item_count,nums);
     for(i=0,t=list;i<item_count;i++,t++)
        {
        endpage(i);
        print_polozka(nums[i],t);
        }
     free(list);
     free(nums);
     }
  }

#pragma aux setmode modify [eax]=\
  "mov  eax,55h"\
  "int  10h"
void setmode();

void help()
  {
  puts("\nUsage: ITEMLIST ATSUZPHGNMC* [pagelen]\n"
       "\n"
       "A - jmeno veci\n"
       "C - cena\n"
       "H - hmotnost\n"
       "N - ID cislo\n"
       "P - popis\n"
       "S - prirazeni zvuku\n"
       "T - typ veci\n"
       "U - umisteni\n"
       "Z - typ zbrane\n"
       "* - Vsechny udaje, odpovida zapisu: ACHNPSTUZ\n"
       "\n"
       "G - Setridi vypis do trid\n"
       "M - Nastavi rezim 132x25zn pro vypis\n"
       "\n"
       "Pagelen - Delka stranky v radkach (min 3), default 0\n"
       "\n"
       "Zapis bez parametru je ekvivaletni zapisu NATS");
  exit(0);
  }

void arguments(char *arg)
  {
  if (!*arg)
     {
     pr_number=1;
     pr_name=1;
     pr_type=1;
     pr_sound=1;
     puts("Parametry: NATS");
     puts("Pouzij ITEMLIST ? pro napovedu\n");
     }
  else
  while (*arg)
     switch(toupper(*arg++))
        {
        case 'A': pr_name=1;break;
        case 'T': pr_type=1;break;
        case 'S': pr_sound=1;break;
        case 'U': pr_umist=1;break;
        case 'Z': pr_zbran=1;break;
        case 'P': pr_popis=1;break;
        case 'H': pr_hmotn=1;break;
        case 'G': grp_sort=1;break;
        case 'N': pr_number=1;break;
        case 'M': setmode();break;
        case 'C': pr_cena=1;break;
        case '*': pr_name=pr_type=pr_sound=pr_umist=pr_zbran=pr_popis=pr_hmotn=pr_number=pr_cena=1;break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':arg--;
                 sscanf(arg,"%d",&pagelen);
                 pagelen-=2;
                 if (pagelen<1) pagelen=0;
                 return;
        default: help();
        }
  }

main(int argc,char **argv)
  {
  int i;
  if (argc<2)arguments("");
  else
     for(i=1;i<argc;i++) arguments(argv[i]);
  load_items_dat("items.dat");
  print_list();
  }
