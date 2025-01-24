// WPathname.cpp: implementation of the WPathname class.
//
//////////////////////////////////////////////////////////////////////

#include "WPathname.h"
#include <windows.h>
#include <shlobj.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WPathname::WPathname(const wchar_t *name /*=NULL*/)
{ 
  if (name) SetPathname(name);
}

WPathname::WPathname(const WString &name)
{
  SetPathname(name);
}

WPathname::WPathname(const wchar_t *relpath, const WPathname &abspath)
{
  _fullpath=WString(relpath);
  int chr=_fullpath.FindLast('\\');
  if (chr!=-1) 
    {
    _path=_fullpath.Left(chr+1);
    _filetitle=_fullpath.Right(chr+1);
    }
  else
  {
    _filetitle=_fullpath;
  }
  chr=_filetitle.FindLast('.');
  if (chr!=-1)
  {
    _extension=_filetitle.Right(chr);
    _filetitle=_filetitle.Left(chr);
  }

  RelativeToFull(abspath);
}


WPathname::WPathname(const WPathname &other)
{
 _fullpath=other._fullpath; 
 _path=other._path;     
 _filetitle=other._filetitle;
 _extension=other._extension;
}

void WPathname::SetDrive(wchar_t dr)
{
  if (HasDrive())
  {
    if (dr==0) _path=_path.Right(2);
    else _path[0]=dr;
  }
  else if (dr!=0)
  {
    int np=IsNetworkPath();
    wchar_t buff[2];
    buff[0]=dr;
    buff[1]=':';
    buff[2]=0;
    if (np)
      _path=WString(buff)+_path.Right(np);
    else
      _path=WString(buff)+_path;
    
  }
 RebuildPath();
}

void WPathname::SetDirectory(const wchar_t *dir)
{
  bool copydrv;       //directory doesn't contain drive, need copy from original
  bool addslash;      //directory doesn't ending by backslash, need add it

  int len=wcslen(dir);
  copydrv=HasDrive(dir) || !HasDrive(); //copy original drive, if exists and directory doesn't contaion drive
  if (wcsncmp(dir,L"\\\\",2)==0) copydrv=true; //network path, don't copy drive
  addslash=len && dir[len-1]!='\\'; //add slash
  if (copydrv)  
    _path=WString(dir);
  else
    _path=_path.Left(2)+WString(dir);
  if (addslash)
    _path+=WSC("\\");
  
  RebuildPath();
}

void WPathname::SetFilename(const wchar_t *filename)
{
  WString wfilename=WString(filename);
  int dot=wfilename.FindLast('.');
  if (dot==-1)
  {
    _filetitle=wfilename;
    _extension=0;
  }
  else
  {
    _filetitle=wfilename.Left(dot);
    _extension=wfilename.Right(dot);
  }

  RebuildPath();
}

void WPathname::SetExtension(const wchar_t *ext)
{
  _extension=WString(ext);
  RebuildPath();
}

void WPathname::SetFileTitle(const wchar_t *title)
{
  _filetitle=WString(title);
  RebuildPath();
}

void WPathname::SetPathname(const wchar_t *pathname)
{
  SetPathname(WString(pathname));
}

void WPathname::SetPathname(const WString &pathname)
{
  if (pathname.GetLength()==0) SetNull();
  else
  {
    wchar_t *part;
    DWORD needsz=GetFullPathNameW(pathname,0,NULL,&part);
    wchar_t *fpth=(wchar_t *)alloca(needsz*sizeof(*fpth));
    GetFullPathNameW(pathname,needsz,fpth,&part);
    part=wcsrchr(fpth,'\\');
    if (part) part++;else part=NULL;
    if (part)
    {
      SetFilename(part);
      *part=0;
    }
    else
      SetFilename(WString());
    SetDirectory(fpth);
  }
}

const wchar_t *WPathname::GetNameFromPath(const wchar_t *path)
{
  const wchar_t *c=wcsrchr(path,'\\');
  if (c!=NULL) c++;else return path;
  return c;
}

const wchar_t *WPathname::GetExtensionFromPath(const wchar_t *path)
{
  const wchar_t *fname=GetNameFromPath(path);  
  const wchar_t *c=wcsrchr(fname,'.');
  if (c==NULL) c=wcsrchr(path,0);
  return c;
}

void WPathname::RebuildPath()
{
  _fullpath=_path+_filetitle+_extension;
}

bool WPathname::GetDirectoryWithDriveWLBS(wchar_t *buff, size_t size) const
{
  size_t psize=wcslen(GetDirectoryWithDrive());
  if (psize>size) return false;
  if (psize==0) {buff[0]=0;return true;}
  wcsncpy(buff,GetDirectoryWithDrive(),psize-1);
  buff[psize-1]=0;
  return true;
}

WString WPathname::GetDirectoryWithDriveWLBS() const
{
  if (_path.GetLength()) return _path.Left(_path.GetLength()-1);
  return WString();
}

bool WPathname::IsPathValid() const
{
  if (IsNull()) return false;  
  wchar_t *invalidChars=L"/*?\"<>|";
  const wchar_t *path=GetFullPath();
  if (*path==0) return false;
  while (*path)
  {
    if (wcschr(invalidChars,*path)!=NULL) return false;
    path++;
  }
  return true;
}

bool WPathname::SetTempDirectory()
{
  wchar_t buff[1];
  DWORD size=GetTempPathW(1,buff);
  if (size==0) return false;  
  WString pth;
  wchar_t *p=pth.CreateBuffer(size);
  if (GetTempPathW(size,p)==0) return false;
  pth.UnlockBuffer();
  _path=pth;
  return true;
}

bool WPathname::SetDirectorySpecial(int nSpecCode)
{
  wchar_t buff[MAX_PATH];
  if (SHGetSpecialFolderPathW(GetForegroundWindow(),buff,nSpecCode,FALSE)!=NOERROR) return false;
  SetDirectory(buff);
  return true;
}

bool WPathname::SetTempFile(const wchar_t *prefix, unsigned int unique)
{
  wchar_t tempname[MAX_PATH];
  if (GetTempFileNameW(GetDirectoryWithDrive(),prefix,unique,tempname)==0) return false;
  this->SetPathname(tempname);
  return true;
}

WPathname WPathname::GetExePath()
{
  wchar_t buff[MAX_PATH*4];
  GetModuleFileNameW(NULL,buff,sizeof(buff));
  return WPathname(buff);
}

const wchar_t *WPathname::FullToRelativeProjectRoot(const wchar_t *full, const wchar_t *projectRoot)
{
  const wchar_t *a=full,*b=projectRoot;
  while (*a && towlower(*a)==towlower(*b)) {a++;b++;};
  if (*b) return full;
  return a;
}

bool WPathname::FullToRelative(const WPathname &relativeto)
{
  if (relativeto.IsNull() || IsNull()) return false;
  bool h1=HasDrive();
  bool h2=relativeto.HasDrive();
  if (h1!=h2) return false;         //rozdilny zpusob adresace - nelze vytvorit relatvni cestu
  if (h1==true && h2==true && towupper(GetDrive())!=towupper(relativeto.GetDrive()))
    return false;       //ruzne disky, nelze vytvorit relativni cestu
  if (wcsncmp(_path,L"\\\\",2)==0) //sitova cesta
  {
    int slsh=0;           //citac lomitek
    const wchar_t *a=_path;
    const wchar_t *b=relativeto._path;
    while (towupper(*a)==towupper(*b) && *a && slsh<3)  //zacatek sitove cesty musi byt stejny
    {
      if (*a=='\\') slsh++;
      a++;b++;
    }
    if (slsh!=3) return false;      //pokud neni stejny, nelze vytvorit relativni cestu
  }
  int sublevel=0;
  const wchar_t *ps1=_path;
  const wchar_t *ps2=relativeto._path;
  if (h1)  {ps1+=2;ps2+=2;}
  const wchar_t *sls=ps2;
  while (towupper(*ps1)==towupper(*ps2) && *ps1) 
  {
    if (*ps2=='\\') sls=ps2+1;
    ps1++;ps2++;
  }
  ps1-=ps2-sls;
  if (sls)
  {    
    while (sls=wcschr(sls,'\\'))
    {
      sls++;
      sublevel++;
    }
  }
  wchar_t *buff=(wchar_t *)alloca((sublevel*3+wcslen(ps1)+1)*sizeof(*buff));
  wchar_t *pos=buff;
  for (int i=0;i<sublevel;i++) 
  {wcscpy(pos,L"..\\");pos+=3;}
  wcscpy(pos,ps1);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

bool WPathname::RelativeToFull(const WPathname &ref)
{
  if (ref.IsNull() || IsNull()) return false;  
  const wchar_t *beg;
  if (HasDrive())
    if (towupper(GetDrive())!=towupper(ref.GetDrive())) return false;
    else beg=_path+2;
  else beg=_path;
  const wchar_t *end=wcschr(ref._path,0);  
  if (beg[0]=='\\')
  {
    int np;
    if (ref.HasDrive()) end=ref._path+2;
    else  if (np=ref.IsNetworkPath()) end=ref._path+np;
    else end=ref._path;
  }
  else while (wcsncmp(beg,L"..\\",3)==0 || wcsncmp(beg,L".\\",2)==0)
  {
    if (beg[1]=='.')
    {
      if (end>ref._path.GetString())
      {
        end--;
        while (end>ref._path.GetString() && end[-1]!='\\') end--;
      }      
      beg+=3;
    }
    else 
      beg+=2;
  }
  int partln=end-ref._path;
  wchar_t *buff=(wchar_t *)alloca((partln+wcslen(beg)+1)*sizeof(*buff));
  wcsncpy(buff,ref._path,partln);
  wcscpy(buff+partln,beg);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

int WPathname::IsNetworkPath() const
{
  if (wcsncmp(_path,L"\\\\",2)==0) //sitova cesta
  {
    const wchar_t *p=_path+2;
    const wchar_t *c=wcschr(p,'\\');
    if (c) return c-_path;
  }
  return 0;
}

void WPathname::SetServerName(const wchar_t *server)
{
  if (HasDrive()) SetDrive(0);
  else
  {
    int np=IsNetworkPath();
    _path=_path.Right(np);
  }

  _path=WSC("\\\\")+WString(server)+WSC("\\");
  RebuildPath();
}

void WPathname::SetNull()
{
  _fullpath=_path=_filetitle=_extension=0;
}

bool WPathname::GetPartFromPath(const wchar_t *path, int partnum, wchar_t *buff, int bufsize, int mode)
{
  const wchar_t *scan=path;
  while (*scan=='\\') scan++;
  while (partnum && *scan)
  {
    while (*scan!='\\' && *scan) scan++;
    while (*scan=='\\') scan++;
    partnum--;
  }
  if (*scan==0) 
  {
    buff[0]=0;
    return false;
  }
  int pt=0;
  if (mode==-1)
  {
    pt=scan-path;
    if (pt>bufsize)
    {
      buff[0]=0;
      return true;
    }
    else           
      memcpy(buff,path,pt);
  }
  bool nlast=false;
  while (*scan && (mode==1 || !nlast) && pt<bufsize)
  {
    buff[pt]=*scan;
    pt++;
    scan++;
    if (*scan=='\\') nlast=true;
  }
  if (pt==bufsize)
  {
    buff[0]=0;
    return true;
  }
  buff[pt]=0;
  return nlast;
}

