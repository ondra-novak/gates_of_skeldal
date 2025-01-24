#include "stdafx.h"
#include ".\ddlfile.h"

DDLFile::DDLFile(void)
{
  _hFile=0;
}

DDLFile::~DDLFile(void)
{
  if (_hFile!=0) CloseHandle(_hFile);
}




bool DDLFile::OpenDDLFile(WString filename)
{
  _hFile=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
    OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,NULL);
  if (_hFile==INVALID_HANDLE_VALUE) 
  {
    _hFile=0;
    return false;
  }
  return true;
}

bool DDLFile::ReadFile(void *data, size_t sz)
{
  DWORD readed=0;
  if (::ReadFile(_hFile,data,sz,&readed,NULL)==FALSE) return false;
  if (readed!=sz) return false;
  return true;
}

bool DDLFile::EnumFiles(IDDLFileEnumerator &enmClass)
{
  unsigned long firstGroup;
  unsigned long groupEndOffset;
  unsigned long endGroups;
  int i;
  int ngroups;
  SetFilePointer(_hFile,0,0,FILE_BEGIN);
  if (ReadFile(&firstGroup,sizeof(firstGroup))==false) return false;
  if (ReadFile(&groupEndOffset,sizeof(firstGroup))==false) return false;
  unsigned long *group=(unsigned long *)alloca(groupEndOffset);
  group[0]=firstGroup;
  group[1]=groupEndOffset;
  ngroups=groupEndOffset/8;
  if (groupEndOffset!=8 && ReadFile(group+2,groupEndOffset-8)==false) return false;
  SetFilePointer(_hFile,12,0,FILE_CURRENT);
  if (ReadFile(&endGroups,sizeof(endGroups))==false) return false;
  for (i=0;i<ngroups && group[i*2+1]<endGroups;i++);
  ngroups=i;
  WString fname;
  for (i=0;i<ngroups;i++)
  {
    unsigned long endGroup=(i+1)<ngroups?group[i*2+3]:endGroups;
    SetFilePointer(_hFile,group[i*2+1],0,FILE_BEGIN);
    unsigned long pos=group[i*2+1];
    while (pos<endGroup)
    {
      char buff[13];
      unsigned long offset;
      if (ReadFile(buff,12)==false) return false;
      if (ReadFile(&offset,4)==false) return false;
      buff[12]=0;
      fname.SetUTF8(buff);
      enmClass.File(fname,group[i*2],offset);
      pos+=12+4;
    }
  }
  return true;
}

unsigned long DDLFile::GetFileSize(unsigned long offset)
{
  unsigned long sz=0;
  SetFilePointer(_hFile,offset,0,FILE_BEGIN);
  ReadFile(&sz,4);
  return sz;
}

DDLData DDLFile::ExtractFile(unsigned long offset)
{
  SetFilePointer(_hFile,offset,0,FILE_BEGIN);
  DDLData data;
  if (ReadFile(&data.sz,4)==false) return DDLData();
  data.data=malloc(data.sz);
  if (ReadFile(data.data,data.sz)==false) return DDLData();
  return data;
}
