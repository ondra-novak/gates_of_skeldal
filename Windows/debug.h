#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>

#define STOP() StopProgram(__FILE__,__LINE__);

static __inline void StopProgram(const char *text, int line) 
  {
  char buff[256];
  sprintf(buff,"Stop at %s line %d",text,line);
  MessageBox(NULL,buff,NULL,MB_OK|MB_SYSTEMMODAL);
  __asm 
    {
    int 3;
    }  
  }

#endif