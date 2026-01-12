#include <dos.h>

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "doserr.h"

void *err_stack;

char err_proc(int error,char disk,char info)
  {
  cprintf("Device error %04X %c %03X \n\r",error,disk+'@',info);
  return 3;
  }

main()
  {
  FILE *f;

  err_stack=malloc(16384);
  install_dos_error(err_proc,(char *)err_stack+16384);
  f=fopen_icase("a:\test","r");
  }
