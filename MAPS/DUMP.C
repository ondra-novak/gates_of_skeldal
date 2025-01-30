#include <skeldal_win.h>
#include <stdio.h>
#include <libs/bgraph.h>
#include <dos.h>
#include <libs/memman.h>

static int latest_version(char *wild,int numpos)
  {
  struct find_t  ff;
  int rc;
  int i=0,j=-1;
  char *p;

  rc=_dos_findfirst(wild,_A_NORMAL,&ff);
  while (rc==0)
     {
     p=ff.name+numpos;
     sscanf(p,"%d",&i);
     if (i>j) j=i;
     rc=_dos_findnext(&ff);
     }
  _dos_findclose(&ff);
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
  i=640*480*3+0x36;
  fwrite(&i,1,4,f);i=0;
  fwrite(&i,1,4,f);
  i=0x36;
  fwrite(&i,1,4,f);
  i=0x28;
  fwrite(&i,1,4,f);
  i=640;
  fwrite(&i,1,4,f);
  i=480;
  fwrite(&i,1,4,f);
  i=1;
  fwrite(&i,1,2,f);
  i=24;
  fwrite(&i,1,2,f);
  i=0;
  fwrite(&i,1,4,f);
  i=640*480*3;
  fwrite(&i,1,4,f);
  for(i=4,r=0;i>0;i--) fwrite(&r,1,4,f);
  for(y=480;y>0;y--)
     {
     a=&screen[(y-1)*640];
     for(x=0;x<640;x++)
        {
        i=a[x];
        b=(i & 0x1f)<<3;
        g=(i & 0x3e0)>>2;
        r=(i & 0x7f00)>>7;
        i=((r*256)+g)*256+b;
        fwrite(&i,1,3,f);
        }
     }
  fclose(f);
  }

