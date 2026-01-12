//Toto je pokus programu pro sklonovani jmen.
//Umi vysklonovat jen jmena z rodu muzskeho zivotneho a zenskeho.
//vse v cisle jednotnem.
//Hodi se do ceskych her pro oslovovani hracu.


//Padovani probiha tak:
/*
  Ke jmenu je treba znat rod: Muzsky/ Zensky

  Pak podle rodu se hleda ve vzorech pro muzky nebo zesky rod:
  Nejprve se porovnaji dva posledni znaky jmeno s mapou znaku u zkoumaneho
vzoru. Pokud tyto znaky souhlasi, muze byt jmeno vysklonovano.
  Pri sklonovani se neprve z koncovky odebere urcity, nebo zadny pocet znaku.
Potom se pripadne upravi konverze dlouheho 'u' s krouzkem na 'o' "Kun"=>"kone".
Nasledne se pripoji koncovka a nakonec se provede kontrola na znak e pred
souhlaskou, ktere je nutne vypustit: Marek => Marka.
  Pri sklonovani muzou ale vzniknout chybne dvojznakove spojeni: treba znak 'n'
s hackem a 'e'. Tyto dvojice jsou vyhledany a nahrazeny dvojici spravnou,
v nasem pripade 'n' a 'e' s hackem.

Ja jen doufam ze tyto upravy maj obecny charakter :-)
Da se prepokladat ze ve svete existuje plno jmen, ktere nelze takto sklonovat.
U nekterych ani neni jasny jak tato jmena sklonovat. Treba
  Bredy, Bredyho, Bredymu, Bredy, Bredy, Bredym, Bredym - Co je to za vzor ???
Uz nemluvim o jmenach cizich.
*/

//Definice knihoven
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef char TPAD[8];
//typ TPAD predstavuje koncovku pro jeden pad

typedef char TZNAKY[40];
//typ TZNAKY predstavuje mapu znaku se kterymi vzor pracuje.
//pokud je na prvnim miste znak '*' tak vzor pracuje se vsemy.

typedef struct vzor
  {
  TZNAKY znak1;
  TZNAKY znak2;
  char chrdel;
  TPAD pady[7];
  }TVZOR;
//struktura TVZOR popisuje sklonovani jednoho vzoru
//znak1 predstavuje mapu znaku pro jmeno v prvnim pade na predposledni pozici
// ... pokud znak neni v mape, nelze jmeno v tomto vzoru sklonovat
//znak2 je jako znak1 s tim ze se testuje znak na posledni pozici
//chrdel je pocet znaku, ktere musi byt odebrany pred pridanim pripony.
//navic pokud je chrdel=0 uplatnuje se konverze Kun -> kone a podobne.
//pady predstavuje jednotlive koncovky pro pady
// pokud je pad roven '-' znamena to ze tvar je schodny jako v 1. pade.

TVZOR zena=
  {
  "hkrdtnbflmpsvz",
  "a�o�u��y�",
  1,
  "-","y","�","u","o","�","ou"
  };
//Vzor pro zena

TVZOR natasa=
  {
  "c��j�����",
  "a�",
  1,
  "-","i","�","u","o","�","ou"
  };
//Vzor pro zena koncici mekkou souhlaskou a a

TVZOR ruze=
  {
  "c��j�����",
  "e�i�o�u��",
  1,
  "-","e","i","i","e","i","�"
  };
//Vzor pro ruze s mekkou koncovkou

TVZOR ruze2=
  {
  "dtn",
  "i��",
  1,
  "-","e","i","i","e","i","�"
  };
//Vzor pro ruze s tvrdou koncovkou d,t,n s mekkou samohlaskou


TVZOR pisen=
  {
  "*",
  "c��j�����flmsx",
  0,
  "-","e","i","-","i","i","�"
  };
//vzor pisen

TVZOR kost=
  {
  "*",
  "hkrdtnbpvz",
  0,
  "-","i","i","-","i","i","�"
  };
//vzor kost

TVZOR pan=
  {
  "*",
  "rdtnbflmpsvz",
  0,
  "-","a","ovi","a","e","ovi","em"
  };
//vzor pan standardni

TVZOR pan2=
  {
  "*",
  "hk",
  0,
  "-","a","ovi","a","u","ovi","em"
  };
//vzor pan pro specialni koncovky h nebo k => pro 5. pad
// Rumburak => Rumburaku / nikoliv Rumburake.


TVZOR quasimodo=
  {
  "hkrdtnbflmpsvz",
  "o",
  1,
  "o","a","ovi","a","o","ovi","em"
  };
//vzor quasimodo resici pripad koncovky '-odo'.
//Odo, oda, odovi, oda, odo, odovi, odem

TVZOR pritel=
  {
  "e�",
  "c��j�����bflmpsvz",
  0,
  "-","e","i","e","i","i","em"
  };
//vzor pritel pro vzor muz pro obojetne souhlasky ze samohlaskou e, �

TVZOR muz=
  {
  "*",
  "c��j�����",
  0,
  "-","e","i","e","i","i","em"
  };
//vzor muz

TVZOR fenix=
  {
  "*",
  "x",
  0,
  "-","e","ovi","e","i","ovi","em"
  };
//vzor pro jmena koncici -x



TVZOR predseda=
  {
  "hkrdtnbpvz",
  "a�o�u��y�",
  1,
  "a","y","ovi","u","o","ovi","ou"
  };
//vzor predseda


TVZOR soudce=
  {
  "c��j�����flmsx",
  "e�i�o�u��",
   1,
  "e","e","i","e","e","i","em"
  };
//vzor soudce


TVZOR soudce2=
  {
  "dtn",
  "�i�",
   1,
  "e","e","i","e","e","i","em"
  };
//vzor soudce pro tvrdou koncovku s mekkou samohlaskou -di -ti -ni
//-de -te -ne.

//Nasledujici funkce vraci 1 pokud jmeno muze byt sklonovano vzorem vz
char test_vzor(TVZOR *vz,char *jmeno)
  {
  char *end;
  char testchar;

  end=strchr(jmeno,0)-2;
  testchar=end[0];
  if (vz->znak1[0]!='*' && strchr(vz->znak1,testchar)==NULL) return 0;
  testchar=end[1];
  if (vz->znak2[0]!='*' && strchr(vz->znak2,testchar)==NULL) return 0;
  return 1;
  }

//tato funkce se vola p�ed odejmut�m ur�it�ho znaku.
//Pokud to toti� je zm�k�ovac� samohl�ska, mus� se p��padn� d,t,n
//zm�k�it p�ed touto samohl�skou.
void odejmi_znak(char *znak)
  {
  if (znak[0]=='�' || znak[0]=='�' || znak[0]=='i')
        switch (znak[-1])
           {
           case 'd': znak[-1]='�';break;
           case 't': znak[-1]='�';break;
           case 'n': znak[-1]='�';break;
           }
  }

//Tato funkce upravuje nektere pady jen maji predpredposledni samohlasku e
//Marek => Marka
//Pisen => Pisne
//Zdenek => Zdenka
void uprav_e_nakonci(char *konec)
  {
  char samohlasky[]="a�e�i�o�u�y�";
  char pismena[]="flmn�r�s�xpv";
  char *c;

  c=konec-2;
  if (strchr(samohlasky,c[2])!=NULL && strchr(samohlasky,c[1])==NULL && strchr(pismena,c[-1])!=NULL)
     if (c[0] =='e'|| c[0]=='�'|| c[0] =='�')
        {
        odejmi_znak(c);
        strcpy(c,c+1);
        }
  }


//Naplni buffer spravnym padem jmena podle vzoru vz
//paduje se od 0 => 1. pad
void ziskej_pad(char *buffer,char *jmeno,TVZOR *vz,char pad)
  {
  char *end;
  strcpy(buffer,jmeno);
  if (vz->pady[pad][0]=='-') return;
  end=strchr(buffer,0)-vz->chrdel;
  odejmi_znak(end); //odejme znak, pokud je => znak 0 neni znak cesky.
  strcpy(end,vz->pady[pad]);
  if (pad && !vz->chrdel && end[-2]=='�') end[-2]='o';
  uprav_e_nakonci(end);
  return;
  }


//Tato funkce upravi nektere specialni dvojice znaku do spravne podoby
//prvni sloupec je puvodni dvojice, druhy sloupec je nova dvojice
void uprav_dvojice(char *jmeno)
  {
  static char *dvojice[]=
        {
        "�e","d�",
        "�e","t�",
        "�e","n�",
        "�i","di",
        "�i","ti",
        "�i","ni",
        "��","d�",
        "��","t�",
        "��","n�",
        "r�","�e",
        "s�","�e",
        "c�","�e",
        "k�","ce",
        "��","�e",
        "��","�e",
        "��","�e",
        "��","�e",
        };

  while (*jmeno)
     {
     int i;
     for(i=0;i<sizeof(dvojice)/sizeof(char *);i+=2)
        {
        if (!strncmp(jmeno,dvojice[i],2))
           {
           strcopy_n(jmeno,dvojice[i+1],2);
           break;
           }
        }
     jmeno++;
     }
  }

//Pole padovych predlozek
char *jmena_padu[]=
  {
  "kdo",
  "bez",
  "k",
  "vid�m",
  "vol�m",
  "o",
  "s"
  };

//Tato funkce zobrazi vypadovane jmeno
void show_table(char *jmeno,TVZOR *vz,char tabnum)
  {
  int i;
  char *buff;

  buff=alloca(strlen(jmeno)+20);
  printf("%d. mo�nost\n\n",tabnum);
  for(i=0;i<7;i++)
     {
     if (i)
        {
        ziskej_pad(buff,jmeno,vz,i);
        uprav_e_nakonci(buff);
        }
        else strcpy(buff,jmeno);
     uprav_dvojice(buff);
     printf("  %d. %-6s %s\n",i+1,jmena_padu[i],buff);
     }
  printf("**** Kl�vesu ****\n");
  getche();
  }

//Tato funkce vypise celkovy vysledek.
void notabs(int i)
  {
  if (i) printf("Program na�el %d mo�most%s.\n",i,(i!=1?(i>1 && i<5?"i":"�"):""));
  else printf("Program bohu�el nena�el ��dnou mo�nost jak sklo�ovat toto jm�no.\n");
  puts("-------------------------------------------------------------------------");
  }

//Tato funkce zobrazi vsechny moznosti padovani pro rod zensky
void hledej_zena(char *jmeno)
  {
  int tabnum=0;

  if (test_vzor(&zena,jmeno)) show_table(jmeno,&zena,++tabnum);
  if (test_vzor(&natasa,jmeno)) show_table(jmeno,&natasa,++tabnum);
  if (test_vzor(&ruze,jmeno)) show_table(jmeno,&ruze,++tabnum);
  if (test_vzor(&ruze2,jmeno)) show_table(jmeno,&ruze2,++tabnum);
  if (test_vzor(&pisen,jmeno)) show_table(jmeno,&pisen,++tabnum);
  if (test_vzor(&kost,jmeno)) show_table(jmeno,&kost,++tabnum);
  notabs(tabnum);
  }

//Tato funkce zobrazi vsechny moznosti padovani pro rod muzsky
void hledej_muz(char *jmeno)
  {
  int tabnum=0;

  if (test_vzor(&pritel,jmeno)) show_table(jmeno,&pritel,++tabnum);
  if (test_vzor(&pan,jmeno)) show_table(jmeno,&pan,++tabnum);
  if (test_vzor(&pan2,jmeno)) show_table(jmeno,&pan2,++tabnum);
  if (test_vzor(&muz,jmeno)) show_table(jmeno,&muz,++tabnum);
  if (test_vzor(&quasimodo,jmeno)) show_table(jmeno,&quasimodo,++tabnum);
  if (test_vzor(&predseda,jmeno)) show_table(jmeno,&predseda,++tabnum);
  if (test_vzor(&soudce,jmeno)) show_table(jmeno,&soudce,++tabnum);
  if (test_vzor(&soudce2,jmeno)) show_table(jmeno,&soudce2,++tabnum);
  if (test_vzor(&fenix,jmeno)) show_table(jmeno,&fenix,++tabnum);
  notabs(tabnum);
  }

//Hlavni cast programu
void hlavni()
  {
  char jmeno[51];
  char pohlavi;
  do
  {
  printf("Zadej jm�no. Vy�aduje se min 3 znaky v �e�tin� kodu kamenick�ch \n");
  printf("Jm�no nesm� m�t v�c ne� 50 znak�, a nesm� obsahovat mezery:\n");
  printf("Pi� mal�mi p�smeny, pouze prvn� pismeno m��e b�t velk�:\n");
  printf("(pouh� 'x' je odchod z programu)\n");
  jmeno_chyba:
  gets(jmeno);
  if (jmeno[0]=='x' && jmeno[1]==0) return;
  if (strlen(jmeno)<3)
     {
     printf("Jm�no mus� b�t min 3 znaky dlouh�\n");
     goto jmeno_chyba;
     }
  chyba:
  printf("Jsi mu� nebo �ena? (M/Z nebo M/F nebo X):");
  pohlavi=getchar();
  while (getchar()!='\n');
  puts("");
  pohlavi=toupper(pohlavi);
  if (pohlavi=='X') return;
  if (pohlavi=='M') hledej_muz(jmeno);
  else if (pohlavi=='F' || pohlavi=='Z') hledej_zena(jmeno);
  else goto chyba;
  }
  while (1);
  }

main()
  {
  hlavni();
  puts("");
  puts("Pokud jsi objevil p�irozen� jm�no (tj re�ln� jm�no), kter� program\n"
       "nedok�zal vysklo�ovat, po�li mi jeho zn�n� na adresu:\n"
       "xnovako1@cs.felk.cvut.cz\n");
  puts("Verze Latin2 se p�ipravuje...\n"
       "O slovensk� verzi se zat�m neuva�uje...\n");
  puts("Napsal: Ond�ej Nov�k za 2 a p�l hodiny ve WATCOM C\n"
       "Zdroj�ky maj� povahu PUBLIC DOMAIN\n");
  }
