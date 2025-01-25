#include <skeldal_win.h>
#include <debug.h>
#include "strlite.c"

#include "devices.h"
#include "event.h"
#include "bmouse.h"
#include "bgraph.h"
#include "gui.h"
#include "strlists.h"


#define SEC(t) (((t) & 0x1f)<<1)
#define MIN(t) (((t)>>5)& 0x3f)
#define HOUR(t) ((t)>>11)
#define DAY(t) ((t) & 0x1f)
#define MONTH(t) (((t)>>5)& 0x0f)
#define YEAR(t) ((t)>>9)

TSTR_LIST read_directory(char *mask,int view_type,int attrs)
  {
  TSTR_LIST flist;
  int index=0;
  char c[2*MAX_PATH];
  WIN32_FIND_DATA s;
  HANDLE h;
  char rc;

  flist=create_list(256);
  if (flist==NULL) return flist;
  h=FindFirstFile(mask,&s);
  if (h!=INVALID_HANDLE_VALUE)
	{
    do 
     {
     char d[MAX_PATH],*p;
     int i=0;

     if (attrs==_A_NORMAL || s.dwFileAttributes & attrs)
        {
        p=d;d[MAX_PATH-1]=0;
/*        if (view_type!=DIR_NAMES)
           {
           while (s.cFileName[i]!='.' && s.cFileName[i]!='\0' ) *p++=s.cFileName[i++];
           if (s.cFileName[i]!='\0') j=i+1;else j=i;
           while (i<8)
              {
              *p++=32;i++;
              }
           i=3;
           *p++='.';
           while (s.name[j]!='\0')
              {
              *p++=s.name[j++];
              i--;
              }
           while (i>0)
              {
              *p++=32;
              i--;
              }
           }
        else */strncpy(d,s.cFileName,MAX_PATH-1);
       switch (view_type)
          {
       case DIR_FULL:sprintf(c,"%s %10d",
                            d,
                            s.nFileSizeLow
                            );
                 break;
        case DIR_SHORT:sprintf(c,"%s %10d",d,s.nFileSizeLow);break;
        case DIR_NAMES:
        case DIR_BREIF:sprintf(c,"%s",d);break;
        }
     if (str_replace(&flist,index++,c)==NULL)
        {
        release_list(flist);
        return NULL;
        }
        }
     rc=FindNextFile(h,&s);
     }
	while (rc);
	}
  FindClose(h);
  if (flist[0]==NULL)
     {
     release_list(flist);
     return NULL;
     }
  sort_list(flist,-1);
  str_delfreelines(&flist);
  return flist;
  }

void name_conv(char *c)
  {
  char *a,*b;

  a=c;b=c;
  while (*a) if (*a==32) a++; else *b++=*a++;
  *b--='\0';
  if (*b=='.') *b='\0';
  }

typedef struct string_list_data
  {
  TSTR_LIST list;
  int topline,maxitems,maxview;
  int selcolor,skipshow;
  int obj_win;
  }
  STRING_LIST_DATA;

void string_list_init(OBJREC *o,int *params)
  {
  STRING_LIST_DATA *p;

  p=(STRING_LIST_DATA *)getmem(sizeof(STRING_LIST_DATA));
  p->topline=0;
  p->list=(TSTR_LIST)*params++;
  p->selcolor=*params++;
  p->skipshow=*params++;
  o->userptr=p;
  p->obj_win=waktual->id;
  }


void string_list_change()
  {
  OBJREC *o1,*o2;
  STRING_LIST_DATA *p;

  o2=o_aktual;
  o1=find_object(waktual,o2->id-1);
  if (o1==NULL) return;
  p=o1->userptr;
  p->topline=f_get_value(0,o2->id);
  redraw_object(o1);
  }

static int get_to_topline(TSTR_LIST ls,int line,int *retu)
  {
  int cnt;
  int ret,i,cn;


  cnt=str_count(ls);ret=0;cn=0;
  for(i=0;i<cnt;i++)
    {
    if (line>=0) ret=i;
    if (ls[i]!=NULL) line--,cn++;
    }
  if (retu!=NULL) *retu=cn;
  return ret;
  }

static int set_line_back(TSTR_LIST ls,int line)
  {
  int i,c=-1,c1=str_count(ls);

  if (line>=c1) line=c1-1;
  for (i=0;i<=line;i++) if (ls[i]!=NULL) c++;
  return c;
  }


void string_list_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  STRING_LIST_DATA *p;
  int xs,ys,y;
  char savech[]=" ";
  int znh,i,j,max;
  TSTR_LIST ls;
  int savcolor=curcolor;

  xs=x2-x1;ys=y2-y1;
  p=o->userptr;
  ls=p->list;
  bar(x1,y1,x2,y2);
  y=y1;p->maxitems=str_count(ls);
  for(j=p->maxitems-1;j>=0;j--) if (ls[j]!=NULL) break;
  j++;
  if (ls)
  {
    do
    {
      i=get_to_topline(ls,p->topline,&max);
      p->maxview=0;
      y=y1;
      do
      {
        char *c;
        int x,j;

        c=ls[i];
        if (c!=NULL && y+(znh=text_height(c))<y2)
        {
          j=p->skipshow;
          while (j-- && *c) c++;
          if (i==*(int *)(o->data))
          {
            curcolor=p->selcolor;
            bar(x1,y,x2,y+znh-1);
          }
          position(x1,y);
          for(j=0,x=x1,savech[0]=c[j];
            c[j]!='\0' && x+text_width(savech)<x2;
            j++,x+=text_width(savech),savech[0]=c[j]
            )
              outtext(savech);
            y+=znh;
            p->maxview++;
        }
        else
          znh=0;
        i++;
      }
      while (y+znh<y2 && i<p->maxitems);
      if (p->topline && y+2*znh<y2 && znh) 
      {
        int dif=y2-(y+znh);
        p->topline-=(dif*2/3)/znh+1;
        curcolor=savcolor;
        bar(x1,y1,x2,y2);
      }
    }
    while (p->topline && y+2*znh<y2 && znh);
  }
  if (waktual->id==p->obj_win)
        {
        OBJREC *ob;

        ob=find_object(waktual,o->id+1);
        if (ob!=NULL)
           {
           send_message(E_GUI,o->id+1,E_CONTROL,0,max-p->maxview,p->maxview);
           ob->events[3]=string_list_change;
           }
        c_set_value(p->obj_win,o->id+1,p->topline);
        }
  }
void string_list_event(EVENT_MSG *msg,OBJREC *o)
  {
  STRING_LIST_DATA *p;
  int y;
  int znh,i;
  TSTR_LIST ls;
  MS_EVENT *ms;
  static char clicked=0;

  p=o->userptr;
  ls=p->list;
  y=o->locy;

  switch (msg->msg)
     {
     case E_MOUSE:
        i=get_to_topline(ls,p->topline,NULL);
        ms=get_mouse(msg);
        curfont=o->font;
        if (ms->tl1 && clicked || ms->event_type & 0x2)
           {
		   if (ls)
              do
              {
              char *c;

              clicked=1;
              c=ls[i];
              if (i>=p->maxitems) return;
              if (c!=NULL) znh=text_height(c); else znh=0;
              if (y+znh>o->locy+o->ys) return;
              if (ms->y>=y && ms->y<y+znh)
                 {
                 if (ls[i]!=NULL) c_set_value(0,o->id,i);else clicked=0;
                 return;
                 }
              i++;
              y+=znh;
              }
           while (1);
           }
        if (ms->event_type & 0x4 && clicked)
           {
            clicked=0;set_change();
           }
        return;
	 case E_KEYBOARD:
	   {
		 int pos=*(int *)(o->data);
		 int  key=(*(int *)msg->data) & 0xff;
		 if (!key)
		 {
		   int save;
		   int curLine;
		   key=*(int *)msg->data >> 8;
		   {
			 do
			 {
			   save=pos;
			   switch (key)
			   {
			   case 'H':pos--;break;
			   case 'P':pos++;break;
			   case 'I':pos-=p->maxview;break;
			   case 'Q':pos+=p->maxview;break;
			   }
			   if (pos<0) pos=0;
			   if (pos>=p->maxitems) pos=p->maxitems-1;
			   curLine=set_line_back(p->list,pos);
			   if (curLine>=p->topline+p->maxview) p->topline=curLine-p->maxview+1;
			   if (curLine<p->topline) p->topline=curLine;
			 }
			 while (save!=pos && p->list[pos]==NULL);
			 c_set_value(0,o->id,pos);
		   }
		 }
		break;
	   }
	   
     case E_CONTROL:
        {
        int *q;

        q=msg->data;
        switch (*q++)
           {
           case 1:p->list=(TSTR_LIST)*q;
                  redraw_object(o);
                  break;
           case 0:*(void **)*q=p->list;
                  break;
           case 2:
                  i=get_to_topline(ls,p->topline,NULL);
                  if (*(int *)o->data<i) p->topline=set_line_back(ls,*(int *)o->data);
                  if (*(int *)o->data>=i+p->maxview) p->topline=set_line_back(ls,*(int *)o->data);
                  break;
           }
        }
     case E_LOST_FOCUS:clicked=0;
     }
  }



void listbox(OBJREC *o)
  {
  o->datasize=sizeof(int);
  o->runs[0]=string_list_init;
  o->runs[1]=string_list_draw;
  o->runs[2]=string_list_event;
  //o->done=string_list_done;
  }

/*void main()
  {
  TSTR_LIST test;
  int i,j;

  test=read_directory("c:\\windows\\system\\*.*",DIR_BREIF,_A_NORMAL);
  j=str_count(test);
  for(i=0;i<j;i++)
     if (test[i]!=NULL) printf("%s\n",test[i]);
  printf("%d souboru.\n",j);
  release_list(test);
  }
*/
