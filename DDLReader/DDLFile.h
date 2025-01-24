#pragma once

#include "WString.h"

class IDDLFileEnumerator
{
public:
  virtual bool File(WString name, int group, unsigned long offset)=0;  
};

struct DDLData
{
  void *data;
  size_t sz;

  DDLData():data(0),sz(0) {}
  ~DDLData() {free(data);}
  DDLData(const DDLData &other):data(other.data),sz(other.sz) 
    {
      DDLData &dother=const_cast<DDLData &>(other);
      dother.data=0;dother.sz=0;
    }
  DDLData& operator =(const DDLData &other) 
    {
      DDLData &dother=const_cast<DDLData &>(other);
      data=other.data;sz=other.sz;
      dother.data=0;dother.sz=0;
      return *this;
    }
};

class DDLFile
{
    HANDLE _hFile;

    bool ReadFile(void *data, size_t sz);
public:
  DDLFile(void);
  ~DDLFile(void);

  bool OpenDDLFile(WString filename);
  bool EnumFiles(IDDLFileEnumerator &enmClass);
  DDLData ExtractFile(unsigned long offset);
  unsigned long GetFileSize(unsigned long offset);
};
