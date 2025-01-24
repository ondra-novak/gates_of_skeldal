#include "stdafx.h"
#include ".\waplayer.h"
#include <malloc.h>

WAPlayer::WAPlayer(void)
{
  _pluginList=0;
}

WAPlayer::~WAPlayer(void)
{
  ClearList();
}


void WAPlayer::ClearList()
{
  delete [] _pluginList;
  _pluginList=0;
  _nPlugins=0;
}

void WAPlayer::LoadPlugins(const char *path)
{
  char *wildcards=(char *)alloca(strlen(path)+10);
  char *pathname=(char *)alloca(strlen(path)+MAX_PATH);
//  CheckPathname(pathname);
  strcpy(wildcards,path);
  strcpy(pathname,path);
  if (wildcards[strlen(wildcards)-1]!='\\') 
    {
      strcat(wildcards,"\\");
      strcat(pathname,"\\");
    }
  char *fname=strchr(pathname,0);
  strcat(wildcards,"in_*.dll");
  HANDLE h;
  ClearList();
  WIN32_FIND_DATA fnd;
  h=FindFirstFile(wildcards,&fnd);
  if (h) do
  {
    _nPlugins++;
  }while(FindNextFile(h,&fnd));
  FindClose(h);

  _pluginList=new WAInputPlugin[_nPlugins];
  int pos=0;
  h=FindFirstFile(wildcards,&fnd);
  if (h != INVALID_HANDLE_VALUE) do
  {
    strcpy(fname,fnd.cFileName);
    if (_pluginList[pos].LoadPlugin(pathname)!=WAInputPlugin::errOk)
      _pluginList[pos].UnloadPlugin();
    else
      pos++;
  }
  while (FindNextFile(h,&fnd));
  _nPlugins=pos;
  FindClose(h);
}

WAInputPlugin *WAPlayer::SelectBestPlugin(const char *songName)
{ 
  for (int i=0;i<_nPlugins;i++)
    if (_pluginList[i].CanPlayFile(songName)==WAInputPlugin::errOk) return _pluginList+i;
  return 0;
}

bool WAPlayer::EnumPlugins(bool (*EnumProc)(WAPlayer &player, WAInputPlugin &plugin, void *context), void *context)
{
  for (int i=0;i<_nPlugins;i++) if (EnumProc(*this,_pluginList[i],context)==false) return false;
  return true;
}