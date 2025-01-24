#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <malloc.h>
#include <memman.h>
#include <event.h>
#include <bmouse.h>
#include <gui.h>
#include <basicobj.h>
#include <zvuk.h>
#include <bgraph.h>
#include "mgifeobj.h"
#include "mgifedit.h"
#include "mgifeact.h"


static void done_bar_init(OBJREC *o,long *params)
  {
  o->userptr=New(long);
  *(long *)o->userptr=*params;
  }

static void done_bar_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  long value,max,x3;

  value=*(long *)o->data;
  max=*(long *)o->userptr;
  x3=x1+(x2-x1)*value/max;
  if (x3<=x1) x3=x1;
  if (x3>=x2) x3=x2;
  bar(x3,y1,x2,y2);
  curcolor=o->f_color[1];
  bar(x1,y1,x3,y2);
  }

void done_bar(OBJREC *o) //define(...done_bar,max);
  {
  o->runs[0]=done_bar_init;
  o->runs[1]=done_bar_draw;
  o->datasize=4;
  }


static void pic_view_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  void *pic;

  x2,y2;
  pic=*(void **)o->data;
  if (pic!=NULL)
     put_picture(x1,y1,pic);
  }

void pic_viewer(OBJREC *o) //define(...pic_view);set_value(ptr)
  {
  o->runs[1]=pic_view_draw;
  o->datasize=4;
  }

//----------------------------------------

#define FRAME_SIZE 2

static void track_view_init(OBJREC *o,void **params)
  {
  o->userptr=New(void *);
  memcpy(o->userptr,params,sizeof(void *));
  }

static void track_view_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  int start;
  int fstart;
  int pos;
  int midl=x1+x2>>1;
  TRACK_DATA_T z,u,*p;

  pos=*(int *)o->data;
  start=(midl-pos*FRAME_SIZE);
  if (start<x1)
     {
     fstart=(x1-start)/FRAME_SIZE;
     start=x1;
     }
  else
     {
     fstart=0;
     bar(x1,y1,start,y2);
     }
  curcolor=0x7fff;
  bar(start,y1,x2,y2);
  curcolor=0x7fe0;
  bar(midl,y1,midl+1,y2);
  p=&z;
  get_vpoint(**(TRACK_DATA_T ***)o->userptr,fstart,&z);
  curcolor=0;
  while(p!=NULL && start+p->time*FRAME_SIZE<=x2)
     {
     int ys1,ys2;

     ys1=y2-(y2-y1)*p->vpoint1/256;
     ys2=y2-(y2-y1)*p->vpoint2/256;
     curcolor=0;
     bar(start-1,ys1-1,start+1,ys1+1);
     curcolor=p->vpoint1==p->vpoint2?0:0x7c00;
     line(start,ys1,start+p->time*FRAME_SIZE,ys2);
     start+=p->time*FRAME_SIZE;
     fstart+=p->time;
     p=p->next;
     }
  if (p!=NULL)
     {
     int ys1,ys2;

     get_vpoint(**(TRACK_DATA_T ***)o->userptr,fstart+(x2-start)/FRAME_SIZE,&u);
     ys1=y2-(y2-y1)*p->vpoint1/256;
     ys2=y2-(y2-y1)*u.vpoint1/256;
     curcolor=0;
     bar(start-1,ys1-1,start+1,ys1+1);
     curcolor=p->vpoint1==p->vpoint2?0:0x7c00;
     line(start,ys1,x2,ys2);
     }
  }

static int find_near_vpoint(TRACK_DATA_T *t,int frame,int yy,int nrf,int nrs)
  {
  int fr=0;
  int nfr=frame,rz=nrf;
  int p,q;
  while (t!=NULL)
     {
     p=abs(fr-frame);
     q=abs(t->vpoint1-yy);
     if (p<rz && q<nrs)
        {
        nfr=fr;
        rz=p;
        }
     fr+=t->time;
     t=t->next;
     }
  return nfr;
  }

static void track_view_click(EVENT_MSG *msg,OBJREC *o)
  {
  MS_EVENT *ms;
  static int last_fr=-1;

  if (msg->msg==E_MOUSE)
     {
     int x,y,*data;
     int frel;
     int frst;
     int fr,nr;
     int yy;

     data=(int *)o->data;
     ms=get_mouse(msg);
     x=ms->x-o->locx;
     y=ms->y-o->locy;
     frel=x/2;
     frst=o->xs/4-*data;
     fr=frel-frst;
     if (fr<0) fr=0;
     if (fr>=total_frames) fr=total_frames-1;
     yy=255-(y*256/o->ys);
     if (yy<0) yy=0;
     nr=find_near_vpoint(**(TRACK_DATA_T ***)o->userptr,fr,yy,5,10*256/o->ys);
     if (ms->event_type & 0x8 && !ms->tl1)
        {
        *data=nr;
        redraw_object(o);
        set_change();
        }
     if (ms->event_type & 0x2) last_fr=nr;
     else
     if (ms->event_type & 0x4) last_fr=-1;
     else
     if (ms->tl1  && last_fr!=-1)
        if (ms->event_type & 0x8)
        {
        delete_vpoint(*(TRACK_DATA_T ***)o->userptr,last_fr);
        redraw_object(o);
        last_fr=-1;
        }
        else
        {
        set_vpoint(*(TRACK_DATA_T ***)o->userptr,last_fr,yy,yy);
        redraw_object(o);
        }
     }
  if (msg->msg==E_LOST_FOCUS) last_fr=-1;
  }

void track_view(OBJREC *o) //track_view(void *track);set_value(pos);
  {
  o->runs[0]=track_view_init;
  o->runs[1]=track_view_draw;
  o->runs[2]=track_view_click;
  o->datasize=4;
  }


#define FRAME_SIZE 2

static void starts_view_init(OBJREC *o,void **params)
  {
  o->userptr=New(void *);
  memcpy(o->userptr,params,sizeof(void *));
  }

static void starts_view_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  int start;
  int fstart;
  int pos;
  int midl=x1+x2>>1;
  int count;
  TRACK_INFO_T *d;
  int *list;
  int i;

  d=*(TRACK_INFO_T **)o->userptr;
  list=d->starts;count=d->starts_count;
  pos=*(int *)o->data;
  start=(midl-pos*FRAME_SIZE);
  if (start<x1)
     {
     fstart=(x1-start)/FRAME_SIZE;
     start=x1;
     }
  else
     {
     fstart=0;
     bar(x1,y1,start,y2);
     }
  curcolor=0x7fff;
  bar(start,y1,x2,y2);
  curcolor=0x7fe0;
  bar(midl,y1,midl+1,y2);
  for(i=0;i<count;i++)
     {
     int y;
     y=list[i]-fstart;
     if (y>=0 && y<(x2-x1)/FRAME_SIZE)
        {
        y*=FRAME_SIZE;
        y+=start;
        curcolor=0;
        bar(y,y1,y+1,y2);
        }
     }
  }

void starts_view(OBJREC *o) //track_view(void *track);set_value(pos);
  {
  o->runs[0]=starts_view_init;
  o->runs[1]=starts_view_draw;
  o->datasize=4;
  }

static void color_box_draw(int x1,int y1,int x2,int y2,OBJREC *o)
  {
  curcolor=*(int *)o->data;
  bar(x1,y1,x2,y2);
  }

void color_box(OBJREC *o)
  {
  o->runs[1]=color_box_draw;
  o->datasize=4;
  }

