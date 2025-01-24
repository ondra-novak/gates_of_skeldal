#include <stdlib.h>
#include <stdio.h>
#include "memman.c"
#include "strlite.c"


TSTR_LIST ls_origin=NULL;
TSTR_LIST ls_new=NULL;

static void load_error_msg(int err,char *filename)
  {
  switch (err)
     {
     case -1: puts ("Source or target file not found");break;
     case -2: puts ("Unexcepted EOF");break;
     case -3: puts ("Internal error in strlite.c");break;
     default: printf("Error in string table at line %d.\n",err);
     }
  printf("File:%s\n",filename);
  exit(1);
  }

void load_lists(char *filename1,char *filename2)
  {
  int err;

  err=load_string_list(&ls_new,filename1);
  if (err) load_error_msg(err,filename1);
  err=load_string_list(&ls_origin,filename2);
  if (err) load_error_msg(err,filename2);
  }


char *create_backup(char *filename)
  {
  char *c,*d;

  c=getmem(strlen(filename)+5);strcpy(c,filename);
  d=strrchr(c,'.');if (d==NULL) d=strchr(c,0);
  strcpy(d,".bak");
  remove(c);
  rename(filename,c);
  return c;
  }

void spoj_stringtable()
  {
  int i;
  int cnt=str_count(ls_new);
  for(i=0;i<cnt;i++) if (ls_new[i]!=NULL && ls_origin[i]==NULL)
     str_replace(&ls_origin,i,ls_new[i]);
  }

static void save_num(FILE *fo,int num)
        {
        char *c=ls_origin[num];

        if (c==NULL) return;
        fprintf(fo,"%d ",num);
        while (*c) if (*c=='\n') (putc('|',fo),c++);else putc(*c++,fo);
        putc('\n',fo);
        }

void save_stringtable(char *filename,char *backup_name)
  {
  FILE *fo,*fb;
  int cnt=str_count(ls_origin);
  int num,rd;
  int oldnum=-1,i;

  fb=fopen(backup_name,"rt");
  if (fb==NULL)
     {
     puts("Cannot open backup file for reading.");
     exit(1);
     }
  fo=fopen(filename,"wt");
  if (fo==NULL)
     {
     puts("Cannot open target file for writting.");
     exit(1);
     }
  num=0;
  rd=fscanf(fb,"%d",&num);
  while (num!=-1)
     {
     if (rd!=1)
        do
           {
           rd=getc(fb);putc(rd,fo);
           }
        while(rd!='\n');
     else
        {
        for(i=oldnum+1;i<=num;i++) save_num(fo,i);
        while((rd=getc(fb))!=EOF && rd!='\n');
        }
     oldnum=num;
     rd=fscanf(fb,"%d",&num);
     }
  for(i=oldnum+1;i<cnt;i++) save_num(fo,i);
  fprintf(fo,"%d",-1);
  fclose(fo);
  fclose(fb);
  }

void main(int argc,char **argv)
  {
  char *back;
  if (argc!=3)
     {
     puts("Usage: ADDTEXT source target");
     exit(1);
     }
  load_lists(argv[1],argv[2]);
  back=create_backup(argv[2]);
  spoj_stringtable();
  save_stringtable(argv[2],back);
  puts("New texts added...");
  }
