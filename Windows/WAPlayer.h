#pragma once

#include "WAInputPlugin.h"

class WAPlayer
{
  WAInputPlugin *_pluginList;
  int _nPlugins;

  WAPlayer &operator=(const WAPlayer &other);
  WAPlayer(const WAPlayer &other);
public:
  WAPlayer(void);
  ~WAPlayer(void);



  void ClearList();
  void LoadPlugins(const char *path);
  WAInputPlugin *SelectBestPlugin(const char *songName);

  bool EnumPlugins(bool (*EnumProc)(WAPlayer &player, WAInputPlugin &plugin, void *context), void *context);
};
