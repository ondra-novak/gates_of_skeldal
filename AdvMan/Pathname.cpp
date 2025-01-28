// Pathname.cpp: implementation of the Pathname class.
//
//////////////////////////////////////////////////////////////////////

#include "Pathname.h"
#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <io.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#pragma comment(lib,"User32")
#pragma comment(lib,"shell32")

Pathname::Pathname(const char *name)
{
  _fullpath=NULL;
  _path=_filetitle=_extension=_end="";

  if (name==NULL) SetPathName("*.*");
  else SetPathName(name);
}

Pathname::Pathname(PathNameNullEnum null)
{
  _fullpath=NULL;
  _path=_filetitle=_extension=_end="";
}


Pathname::Pathname(const char *relpath, const Pathname &abspath)
{
  _path=_filetitle=_extension=_end="";
  _fullpath=NULL;

  char *part;
  char *pth=strcpy((char *)alloca(sizeof(*relpath)+strlen(relpath)+1),relpath);
  part=strrchr(pth,'\\');  
  if (part) part++;else part=NULL;
  if (part)
  {
    SetFilename(part);
    *part=0;
    SetDirectory(pth);
  }
  else
    SetFilename(pth);
  if (RelativeToFull(abspath)==false) SetNull();
}


Pathname::~Pathname()
{
  delete [] _fullpath;
}

const char *Pathname::GetNameFromPath(const char *path)
{
  char *c=strrchr(path,'\\');
  if (c!=NULL) c++;else return path;
  return c;
}

const char *Pathname::GetExtensionFromPath(const char *path)
{
  const char *fname=GetNameFromPath(path);  
  char *c=strrchr(fname,'.');
  if (c==NULL) c=strrchr(path,0);
  return c;
}

void Pathname::RebuildData(const char *path, const char *filetitle, const char *extension, int pathlen, int titlelen, int extlen)
{
  int totalsize=(pathlen+titlelen+extlen)*2+10;
  char *olddata=_fullpath;
  _fullpath=new char[totalsize];
  _path=_fullpath+pathlen+titlelen+extlen+1;
  memcpy(_path,path,pathlen+1);
  _filetitle=_path+pathlen+1;
  memcpy(_filetitle,filetitle,titlelen+1);
  _extension=_filetitle+titlelen+1;
  memcpy(_extension,extension,extlen+1);
  _end=_extension+extlen+1;
  RebuildPath();
  delete [] olddata;
}

void Pathname::RebuildPath()
{
  sprintf(_fullpath,"%s%s%s",_path,_filetitle,_extension);
}

void Pathname::SetDrive(const char dr)
{
  if (HasDrive())
  {
    if (dr==0)
    {
      strcpy(_path,_path+2);
      strcpy(_fullpath,_fullpath+2);
    }
    else
    {
      _path[0]=dr;
      _fullpath[0]=dr;
    }
  }
  else if (dr!=0)
  {        
    int np=IsNetworkPath();
    if (np)
    {
      _path[0]=dr;
      _path[1]=':';
      strcpy(_path+2,_path+np);
      RebuildPath();
    }
    else
    {
      char *c=(char *)alloca((strlen(_path)+4)*sizeof(*c));          
      sprintf(c,"%c:%s",dr,_path);
      SetDirectory(c);
    }
  }
}

void Pathname::SetDirectory(const char *dir)
{  
  bool copydrv;       //directory doesn't contain drive, need copy from original
  bool addslash;      //directory doesn't ending by backslash, need add it

  int len=strlen(dir);
  copydrv=HasDrive() && !HasDrive(dir); //copy original drive, if exists and directory doesn't contaion drive
  if (strncmp(dir,"\\\\",2)==0) copydrv=false; //network path, don't copy drive
  addslash=len && dir[len-1]!='\\'; //add slash
  if (addslash || copydrv)
  {
    char *c=(char *)alloca((len+4)*sizeof(dir[0])); //allocate some space for string
    if (addslash && copydrv)
      sprintf(c,"%c%c%s\\",_path[0],_path[1],dir);  //add drive and add slash
    else if (addslash)
      sprintf(c,"%s\\",dir);                        //add slash only
    else 
      sprintf(c,"%c%c%s",_path[0],_path[1],dir);    //add drive only
    dir=c;                    //this is new path for now
    len=strlen(dir);
  }
  if (len<_filetitle-_path)   //there is space for store path
  {strcpy(_path,dir); RebuildPath();} //store it and rebuild result
  else    
    RebuildData(dir,_filetitle,_extension,len,strlen(_filetitle),strlen(_extension));
  //rebuild internal data complettly
}

void Pathname::SetFilename(const char *filename)
{
  char *dot=strrchr(filename,'.');
  if (dot==NULL)
  {
    SetFiletitle(filename);
    SetExtension("");
    return;
  }
  int tllen=dot-filename;
  int exlen=strlen(dot);
  char *c=(char *)alloca((tllen+1)*sizeof(*c));
  memcpy(c,filename,tllen);
  c[tllen]=0;
  if (exlen+tllen+1<_end-_filetitle)
  {
    memcpy(_filetitle,c,tllen+1);
    _extension=_filetitle+tllen+1;
    memcpy(_extension,dot,exlen+1);
    RebuildPath();
  }
  else
    RebuildData(_path,c,dot,strlen(_path),tllen,exlen);
}

void Pathname::SetExtension(const char *ext)
{
  int len=strlen(ext);
  if (ext[0] && ext[0]!='.')
  {
    char *s=(char *)alloca((len+2)*sizeof(*s));
    sprintf(s,".%s",ext);
    ext=s;
    len++;
  }
  if (len<_end-_extension)
  {
    memcpy(_extension,ext,len+1);
    RebuildPath();
  }
  else
  {
    RebuildData(_path,_filetitle,ext,strlen(_path),strlen(_filetitle),len);
  }
}

void Pathname::SetFiletitle(const char *title)
{
  int len=strlen(title);
  if (len<_extension-_filetitle)
  {
    memcpy(_filetitle,title,len+1);
    RebuildPath();
  }
  else
  {
    RebuildData(_path,title,_extension,strlen(_path),len,strlen(_extension));
  }
}

void Pathname::SetPathName(const char *pathname)
{
  if (pathname==NULL || pathname[0]==0) 
  {
    SetNull();
    return;
  }
  char *part;
  DWORD needsz=GetFullPathName(pathname,0,NULL,&part);
  char *fpth=(char *)alloca(needsz*sizeof(*fpth));
  GetFullPathName(pathname,needsz,fpth,&part);
  part=strrchr(fpth,'\\');
  if (part) part++;else part=NULL;
  if (part)
  {
    SetFilename(part);
    *part=0;
  }
  else
    SetFilename("");
  SetDirectory(fpth);
}

Pathname& Pathname::operator=(const Pathname& other)
{
  if (other.IsNull()) SetNull();
  else RebuildData(other._path,other._filetitle,other._extension,strlen(other._path),strlen(other._filetitle),strlen(other._extension));
  return *this;
}

Pathname::Pathname(const Pathname &other)
{
  _fullpath=NULL;
  if (other.IsNull()) SetNull();
  else RebuildData(other._path,other._filetitle,other._extension,strlen(other._path),strlen(other._filetitle),strlen(other._extension));  
}

bool Pathname::FullToRelative(const Pathname &relativeto)
{
  if (relativeto.IsNull() || IsNull()) return false;
  bool h1=HasDrive();
  bool h2=relativeto.HasDrive();
  if (h1!=h2) return false;         //rozdilny zpusob adresace - nelze vytvorit relatvni cestu
  if (h1==true && h2==true && toupper(GetDrive())!=toupper(relativeto.GetDrive()))
    return false;       //ruzne disky, nelze vytvorit relativni cestu
  if (strncmp(_path,"\\\\",2)==0) //sitova cesta
  {
    int slsh=0;           //citac lomitek
    const char *a=_path;
    const char *b=relativeto._path;
    while (toupper(*a)==toupper(*b) && *a && slsh<3)  //zacatek sitove cesty musi byt stejny
    {
      if (*a=='\\') slsh++;
      a++;b++;
    }
    if (slsh!=3) return false;      //pokud neni stejny, nelze vytvorit relativni cestu
  }
  int sublevel=0;
  const char *ps1=_path;
  const char *ps2=relativeto._path;
  if (h1) 
  {ps1+=2;ps2+=2;}
  const char *sls=ps2;
  while (toupper(*ps1)==toupper(*ps2) && *ps1) 
  {
    if (*ps2=='\\') sls=ps2+1;
    ps1++;ps2++;
  }
  ps1-=ps2-sls;
  if (sls)
  {    
    while (sls=strchr(sls,'\\'))
    {
      sls++;
      sublevel++;
    }
  }
  char *buff=(char *)alloca((sublevel*3+strlen(ps1)+1)*sizeof(*buff));
  char *pos=buff;
  for (int i=0;i<sublevel;i++) 
  {strcpy(pos,"..\\");pos+=3;}
  strcpy(pos,ps1);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

bool Pathname::RelativeToFull(const Pathname &ref)
{
  if (ref.IsNull() || IsNull()) return false;  
  const char *beg;
  if (HasDrive())
    if (toupper(GetDrive())!=toupper(ref.GetDrive())) return false;
    else beg=_path+2;
  else beg=_path;
  const char *end=strchr(ref._path,0);  
  if (beg[0]=='\\')
  {
    int np;
    if (ref.HasDrive()) end=ref._path+2;
    else  if (np=ref.IsNetworkPath()) end=ref._path+np;
    else end=ref._path;
  }
  else while (strncmp(beg,"..\\",3)==0 || strncmp(beg,".\\",2)==0)
  {
    if (beg[1]=='.')
    {
      if (end>ref._path)
      {
        end--;
        while (end>ref._path && end[-1]!='\\') end--;
      }      
      beg+=3;
    }
    else 
      beg+=2;
  }
  int partln=end-ref._path;
  char *buff=(char *)alloca((partln+strlen(beg)+1)*sizeof(*buff));
  memcpy(buff,ref._path,partln);
  strcpy(buff+partln,beg);
  SetDrive(0);
  SetDirectory(buff);
  return true;
}

int Pathname::IsNetworkPath() const
{
  if (strncmp(_path,"\\\\",2)==0) //sitova cesta
  {
    const char *p=_path+2;
    char *c=strchr(p,'\\');
    if (c) return c-_path;
  }
  return 0;
}

void Pathname::SetServerName(const char *server)
{
  if (HasDrive()) SetDrive(0);
  else
  {
    int np=IsNetworkPath();
    if (np) strcpy(_path,_path+np); //str
  }
  char *buff=(char *)alloca((strlen(server)+strlen(_path)+5)*sizeof(*buff));
  if (_path[0]!='\\')
    sprintf(buff,"\\\\%s\\%s",server,_path);
  else
    sprintf(buff,"\\\\%s%s",server,_path);
  SetDirectory(buff);
}

void Pathname::SetNull()
{
  delete [] _fullpath;
  _fullpath=NULL;
  _path=_filetitle=_extension=_end="";
}

bool Pathname::GetPartFromPath(const char *path, int partnum, char *buff, int bufsize, int mode)
{
  const char *scan=path;
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

bool Pathname::GetDirectoryWithDriveWLBS(char *buff, size_t size) const
{
  size_t psize=strlen(GetDirectoryWithDrive());
  if (psize>size) return false;
  if (psize==0) {buff[0]=0;return true;}
  strncpy(buff,GetDirectoryWithDrive(),psize-1);
  buff[psize-1]=0;
  return true;
}

bool Pathname::IsPathValid() const
{
  if (IsNull()) return false;  
  char *invalidChars="/*?\"<>|";
  const char *path=GetFullPath();
  if (*path==0) return false;
  while (*path)
  {
    if (strchr(invalidChars,*path)!=NULL) return false;
    path++;
  }
  return true;
}



bool Pathname::SetTempDirectory()
{
  char buff[1];
  DWORD size=GetTempPath(1,buff);
  if (size==0) return false;
  size++;
  char *p=(char *)alloca(size);
  if (GetTempPath(size,p)==0) return false;
  SetDirectory(p);
  return true;
}

bool Pathname::SetDirectorySpecial(int nSpecCode)
{
  char buff[MAX_PATH];
  if (SHGetSpecialFolderPath(GetForegroundWindow(),buff,nSpecCode,FALSE)!=NOERROR) return false;
  SetDirectory(buff);
  return true;
}

bool Pathname::SetTempFile(const char *prefix, unsigned int unique)
{
  char tempname[MAX_PATH];
  if (GetTempFileName(GetDirectoryWithDrive(),prefix,unique,tempname)==0) return false;
  this->SetPathName(tempname);
  return true;
}

Pathname Pathname::GetExePath()
{
  char buff[MAX_PATH*4];
  GetModuleFileName(NULL,buff,sizeof(buff));
  return Pathname(buff);
}

const char *Pathname::FullToRelativeProjectRoot(const char *full, const char *projectRoot)
{
  const char *a=full,*b=projectRoot;
  while (*a && tolower(*a)==tolower(*b)) {a++;b++;};
  if (*b) return full;
  return a;
}

bool Pathname::CreateFolder(void *security_descriptor)
{
  int begpart=-1;
  int len=strlen(_fullpath)+1;
  char *buff=(char *)alloca(len);
  for (int i=1;GetPart(i,buff,len,-1);i++)
  {
    if (begpart==-1 && _check_file_exists(buff)!=0) begpart=i;
    if (begpart!=-1)
    {
      if (begpart==-1) begpart=i;
      BOOL res=CreateDirectory(buff,(LPSECURITY_ATTRIBUTES)security_descriptor);
      if (res==FALSE)
      {
        for (int j=i;i>=begpart;i--)
        {
          GetPart(i,buff,len,-1);
          RemoveDirectory(buff);
        }
        return false;
      }
    }
  }
  return true;
}

bool Pathname::CreateFolder(const char *path, void *security_descriptor)
{
  Pathname pth;
  pth.SetDirectory(path);
  return pth.CreateFolder(security_descriptor);
}

static bool RemoveDirectoryFull(const Pathname &p, int part, char *buff, int bufsize)
{  
  if (p.GetPart(part+1,buff,bufsize,-1))
    if (RemoveDirectoryFull(p,part+1,buff,bufsize)==false) return false;
  p.GetPart(part,buff,bufsize, -1);
  return RemoveDirectory(buff)!=FALSE;
}

static bool RemoveDirRecursive(const char *dir, const char *mask)
{
  if (dir==0 || dir[0]==0) return false;
  if (strcmp(dir,"\\")==0) return false;
  bool res=true;
  Pathname newp;
  newp.SetDirectory(dir);
  if (mask)
  {
    WIN32_FIND_DATAA fnd;
    HANDLE h;
    newp.SetFilename(mask);
    h=FindFirstFileA(newp,&fnd);
    if (h) 
    {
      do
      {
        if (!(fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
          newp.SetFilename(fnd.cFileName);
          if (DeleteFile(newp)==FALSE) res=false;
        }
      }while (FindNextFileA(h,&fnd));
      CloseHandle(h);
    }
  }
  {
    WIN32_FIND_DATAA fnd;
    HANDLE h;
    newp.SetFilename("*.*");
    h=FindFirstFileA(newp,&fnd);
    if (h) 
    {
      do
      {
        if ((fnd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(fnd.cFileName,".")!=0 && strcmp(fnd.cFileName,"..")!=0)
        {
          newp.SetFilename(fnd.cFileName);
          if (RemoveDirRecursive(newp,mask)==false) res=false;
          else DeleteFile(newp);
        }
      }while (FindNextFileA(h,&fnd));
      CloseHandle(h);
    }
  }
  return res;

}

bool Pathname::DeleteFolder(int dfFlags)
{
  int bufflen=strlen(_fullpath)+1;
  char *buff=(char *)alloca(bufflen);
/*  if (dfFlags & DFRecycleBin)
  {
    GetDirectoryWithDriveWLBS(buff,bufflen);
    SHFILEOPSTRUCT delinfo;
    delinfo.hwnd=NULL;
    delinfo.wFunc=FO_DELETE;
    delinfo.pFrom=GetFullPath();
    delinfo.pTo=NULL;
    delinfo.fFlags=FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_NOERRORUI|((dfFlags & DFRecursive)?0:FOF_NORECURSION)|
      ((dfFlags & DFShowProgress)?0:FOF_SILENT);
    delinfo.fAnyOperationsAborted=0;
    delinfo.hNameMappings=0;
    delinfo.lpszProgressTitle=0;

  }
  else*/
  {
    if (dfFlags & DFRecursive)
    {
      bool res=RemoveDirRecursive(GetDirectoryWithDrive(),dfFlags & DFFile?GetFilename():0);
      if (res==false) return false;
    }
    if (dfFlags & DFPath)
    {
      if (GetPart(1,buff,bufflen,-1)==false) return false;
      return RemoveDirectoryFull(*this, 1,buff,bufflen);
    }
    else
    {
      GetDirectoryWithDriveWLBS(buff,bufflen);
      return RemoveDirectory(buff)!=FALSE;
    }    

  }
  return false;
}