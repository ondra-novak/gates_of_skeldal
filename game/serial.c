#include <stdio.h>

#define DELITEL 0xC005


unsigned long vysledek;

main()
  {
  int i;
  for(i=0;i<65535;i++)
     {
     vysledek=DELITEL*i;
     printf("%08X  ",vysledek);
     }
  }
