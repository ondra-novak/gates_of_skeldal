#include <stdio.h>
#include <mem.h>

unsigned long l;

#define ZAKLAD_CRC 0xC005

char data[100000];
long delka;
FILE *f;


main()
  {
  int i;
  f=fopen("CRC.C","rb");
  memset(data,0,sizeof(data));
  delka=fread(data,1,sizeof(data),f);
  fclose(f);
  memcpy(&l,data,2);i=0;
  l%=ZAKLAD_CRC;
  while(i<delka)
     {
     i+=2;
     l<<=16;
     memcpy(&l,data+i,2);
     l%=ZAKLAD_CRC;
     }
  printf("CRC: %x\n",l);
  }
z
