#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <vesa.h>
#include <i86.h>
#include "bgraph.h"

void *xlat;

static void help(void)
  {
  puts("\n"
    "Tento program odtestuje grafickou kartu a rozhodne zda lze pouzit bez\n"
    "dodatecne podpory ovladace UNIVESA ci UNIVBE.\n"
    "\n"
    "Program ma jednoduche ovladani: \n\n"
    "       Esc - pokud rezim nevyhovuje (nebo obrazovka je cerna ci plna nesmyslu)\n"
    "       Enter - pokud rezim vyhovuje.\n"
    "\n"
    "Z dosavadnich pokusu se ukazuje ze program muze mit problemy na kartach rady\n"
    "CIRRUS LOGIC a klony\n"
    "\n"
    "Stiskni ENTER a zacneme."
     );
  getchar();
  }

extern word sada7;

#define TEXT_OK "Stiskni ENTER pokud re‘im vyhovuje"

void testovaci_obrazec(char *name)
  {
  int i,j,k,x,y,c;
  curcolor=0;
  bar(0,0,639,479);
  rectangle(0,0,639,479,0x7fff);
  line(0,0,639,479);
  line(0,479,639,0);
  x=20,y=20;
  for(k=0;k<7;k++)
     {
     for(i=0;i<64;i++)
        for(j=0;j<64;j++)
        {
        switch (k)
           {
           case 0:c=(i>>1)+((j>>1)<<5);break;
           case 1:c=(i>>1)+((j>>1)<<10);break;
           case 2:c=((i>>1)<<5)+((j>>1)<<10);break;
           case 3:c=(i>>1)+((j>>1)<<5)+0x7c00;break;
           case 4:c=(i>>1)+((j>>1)<<10)+0x03e0;break;
           case 5:c=((i>>1)<<5)+((j>>1)<<10)+0x001f;break;
           case 6:c=(i+j)>>2;c=(c<<10)+(c<<5)+c;break;
           }
        point(x+i,y+j,c);
        }
     x+=100;
     if (x>300) {y+=100;x=20;}
     }
  curfont=&sada7;
  set_aligned_position(320,400,1,0,name);
  outtext(name);
  set_aligned_position(320,420,1,0,TEXT_OK);
  outtext(TEXT_OK);
  showview(0,0,0,0);
  }

void write_error(int error,char *rezim)
  {
  switch (error)
     {
     case -1: printf("Graficky rezim karta nepodporuje (%s)\n",rezim);break;
     case -10: printf("Rezim nepodporuje zmenu scanovaci radky (%s)\n",rezim);break;
     }
  puts("\n Stiskni cokoliv a budem pokracovat");
  }

void snd(int freq)
  {
  sound(freq);
  delay(100);
  nosound();
  }

int test_hicolor_1(void)
  {
  int error;
  error=initmode32();
  if (error)
     {
     closemode();
     write_error(error,"Hicolor");
     return error;
     }
  testovaci_obrazec("640 x 480 x 32768 barev");
  snd(100);
  while (kbhit()) getche();
  error=getche()==27;
  snd(1000);
  closemode();
  return error;
  }


int test_palcolor_1(void)
  {
  int error;
  error=initmode256(xlat);
  if (error)
     {
     closemode();
     write_error(error,"256 barev");
     return error;
     }
  testovaci_obrazec("640 x 480 x 256 barev / rastrovani / paleta 5,6,5");
  snd(100);
  while (kbhit()) getche();
  error=getche()==27;
  snd(1000);
  closemode();
  return error;
  }



main()
  {
  char ok1,ok2;

  xlat=create_special_palette();
  help();
  ok1=test_hicolor_1();
  ok2=test_palcolor_1();
  if (ok1 && ok2)
     {
     puts("Hru 'Brany Skeldalu' nelze provozovat na teto karte bez ovladace");
     }
  else
     {
     if (!ok1) puts("Graficka karta JE kompatibilni s VESA 1.2");
     if (!ok2) puts("Graficka karta JE kompatibilni s VESA 1.0.");
     puts("Hru lze spustit bez univerzalniho ovladace");
     }
  puts("\n"
       "Dotazy na e-mail: xnovako1@cs.felk.cvut.cz");

  }
