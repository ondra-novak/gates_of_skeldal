#include <skeldal_win.h>
#include <stdio.h>
#include <bgraph.h>
#include <dos.h>
#include <memman.h>
#include <debug.h>

static int latest_version(char *wild,int numpos)
  {
  WIN32_FIND_DATA ff;
  HANDLE rc;
  int i=0,j=-1;
  char *p;

  rc=FindFirstFile(wild,&ff);
  if (rc!=INVALID_HANDLE_VALUE)
    do
     {
     p=ff.cFileName+numpos;
     sscanf(p,"%d",&i);
     if (i>j) j=i;
     }
    while (FindNextFile(rc,&ff));
  FindClose(rc);
  return j;  
  }

void save_dump()
  {
  static dump_counter=-1;
  FILE *f;
  int i,r,g,b,x,y;
  word *a;
  char c[20];

  if (dump_counter==-1)
     {
     dump_counter=latest_version("DUMP*.BMP",4);
     SEND_LOG("(DUMP) Dump counter sets to %d",dump_counter,0);
     }
  sprintf(c,"DUMP%04d.BMP",++dump_counter);
  SEND_LOG("(DUMP) Saving screen shot named '%s'",c,0);
  f=fopen(c,"wb");
  fputc('B',f);fputc('M',f);
  i=DxGetResX()*DxGetResY()*3+0x36;
  fwrite(&i,1,4,f);i=0;
  fwrite(&i,1,4,f);
  i=0x36;
  fwrite(&i,1,4,f);
  i=0x28;
  fwrite(&i,1,4,f);
  i=DxGetResX();
  fwrite(&i,1,4,f);
  i=DxGetResY();
  fwrite(&i,1,4,f);
  i=1;
  fwrite(&i,1,2,f);
  i=24;
  fwrite(&i,1,2,f);
  i=0;
  fwrite(&i,1,4,f);
  i=DxGetResX()*DxGetResY()*3;
  fwrite(&i,1,4,f);
  for(i=4,r=0;i>0;i--) fwrite(&r,1,4,f);
  for(y=DxGetResY();y>0;y--)
     {
     word *scr=GetScreenAdr();
     a=scr+(y-1)*scr_linelen2;
     for(x=0;x<DxGetResX();x++)
        {
        i=a[x];
        b=(i & 0x1f)<<3;
        g=(i & 0x7ff)>>3;
        r=i>>8;
        i=((r*256)+g)*256+b;
        fwrite(&i,1,3,f);
        }
     }
  fclose(f);
  }

