#include <skeldal_win.h>
#include <commdlg.h>
#include "install.h"
#include "resource.h"
#include <string.h>
#include <SHLOBJ.H>
extern "C"
  {
#include <libs/strlite.h>
#include <libs/inicfg.h>
  }
#include <malloc.h>
#include <stdio.h>

static void correctfilter(char *z)
  {
  char *c;
  while ((c=strrchr(z,'|'))!=NULL) *c=0;
  }

static void CommDlgError(HWND owner, int error)
{
  char *msg=0;
  switch (error)
  {
  case CDERR_DIALOGFAILURE: msg="CDERR_DIALOGFAILURE: The dialog box could not be created. The common dialog box function's call to the DialogBox function failed. For example, this error occurs if the common dialog box call specifies an invalid window handle. ";break;
  case CDERR_FINDRESFAILURE: msg="CDERR_FINDRESFAILURE: The common dialog box function failed to find a specified resource. ";break;
  case CDERR_INITIALIZATION: msg="CDERR_INITIALIZATION: The common dialog box function failed during initialization. This error often occurs when sufficient memory is not available. ";break;
  case CDERR_LOADRESFAILURE: msg="CDERR_LOADRESFAILURE: The common dialog box function failed to load a specified resource. ";break;
  case CDERR_LOADSTRFAILURE: msg="CDERR_LOADSTRFAILURE: The common dialog box function failed to load a specified string. ";break;
  case CDERR_LOCKRESFAILURE: msg="CDERR_LOCKRESFAILURE: The common dialog box function failed to lock a specified resource. ";break;
  case CDERR_MEMALLOCFAILURE: msg="CDERR_MEMALLOCFAILURE: The common dialog box function was unable to allocate memory for internal structures. ";break;
  case CDERR_MEMLOCKFAILURE: msg="CDERR_MEMLOCKFAILURE: The common dialog box function was unable to lock the memory associated with a handle. ";break;
  case CDERR_NOHINSTANCE: msg="CDERR_NOHINSTANCE: The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding instance handle. ";break;
  case CDERR_NOHOOK: msg="CDERR_NOHOOK: The ENABLEHOOK flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a pointer to a corresponding hook procedure. ";break;
  case CDERR_NOTEMPLATE: msg="CDERR_NOTEMPLATE: The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding template. ";break;
  case CDERR_REGISTERMSGFAIL: msg="CDERR_REGISTERMSGFAIL: The RegisterWindowMessage function returned an error code when it was called by the common dialog box function. ";break;
  case CDERR_STRUCTSIZE: msg="CDERR_STRUCTSIZE: The lStructSize member of the initialization structure for the corresponding common dialog box is invalid. ";break;
  case FNERR_BUFFERTOOSMALL: msg="FNERR_BUFFERTOOSMALL: The buffer pointed to by the lpstrFile member of the OPENFILENAME structure is too small for the file name specified by the user. The first two bytes of the lpstrFile buffer contain an integer value specifying the size, in TCHARs, required to receive the full name.  ";break;
  case FNERR_INVALIDFILENAME: msg="FNERR_INVALIDFILENAME: A file name is invalid. ";break;
  case FNERR_SUBCLASSFAILURE: msg="FNERR_SUBCLASSFAILURE: An attempt to subclass a list box failed because sufficient memory was not available. ";;break;
  default: msg="Unknown CommDlgError.";break;
  }
  char *text=(char *)alloca(strlen(msg)+200);
  sprintf(text,"CommDlgError (%d / %04X)\r\n\r\nDescription:\r\n\r\n%s",error,error,msg);
  MessageBox(owner,text,NULL,MB_OK|MB_ICONEXCLAMATION);
}

static bool VyberSlozku(HWND owner, char *slozka)
  {
  OPENFILENAME ofn;
  char filter[100];
  char title[100];  
  LoadString(GetModuleHandle(NULL),IDS_INSTALLFILTER,filter,sizeof(filter));
  LoadString(GetModuleHandle(NULL),IDS_INSTALLTITLE,title,sizeof(title));
  LoadString(GetModuleHandle(NULL),IDS_INSTALLPRENAME,slozka,MAX_PATH);
  correctfilter(filter);
  memset(&ofn,0,sizeof(ofn));
#ifdef OPENFILENAME_SIZE_VERSION_400
  ofn.lStructSize=OPENFILENAME_SIZE_VERSION_400;
#else
  ofn.lStructSize=sizeof(ofn);
#endif
  ofn.hwndOwner=owner;
  ofn.lpstrFilter=filter;
  ofn.lpstrFile=slozka;
  ofn.nMaxFile=MAX_PATH*4;
  ofn.lpstrTitle=title;
  ofn.Flags=OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
  if (GetOpenFileName(&ofn)) return true;
  else 
  {
    int err=CommDlgExtendedError();
    if (err) CommDlgError(owner,err);
    return false;
  }
  }

static bool VyberCD(HWND owner, char *slozka)
  {
  OPENFILENAME ofn;
  char filter[100];
  char title[100];  
  LoadString(GetModuleHandle(NULL),IDS_INSTALLCDFILTER,filter,sizeof(filter));
  LoadString(GetModuleHandle(NULL),IDS_INSTALLCDTITLE,title,sizeof(title));
  LoadString(GetModuleHandle(NULL),IDS_INSTALLCDFILE,slozka,MAX_PATH);
  correctfilter(filter);
  memset(&ofn,0,sizeof(ofn));
#ifdef OPENFILENAME_SIZE_VERSION_400
  ofn.lStructSize=OPENFILENAME_SIZE_VERSION_400;
#else
  ofn.lStructSize=sizeof(ofn);
#endif
  ofn.hwndOwner=owner;
  ofn.lpstrFilter=filter;
  ofn.lpstrFile=slozka;
  ofn.nMaxFile=MAX_PATH*4;
  ofn.lpstrTitle=title;
  ofn.Flags=OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
  if (GetOpenFileName(&ofn)) return true;
  else  {
    int err=CommDlgExtendedError();
    if (err) CommDlgError(owner,err);
    return false;
  }
  }



static TSTR_LIST filetocopy=NULL;

static void FileSingleCopy(const char *src, const char *trg)
  {
  if (filetocopy==NULL) filetocopy=create_list(100);
  str_add(&filetocopy,(char *)src);
  str_add(&filetocopy,(char *)trg);
  }

static void FileCopy(HWND hWnd,const char *srcmask, const char *trgmask, const char *src, const char *trg)
  {
  char *fsrc=(char *)alloca((strlen(srcmask)+strlen(src))*2+MAX_PATH);
  char *ftrg=(char *)alloca((strlen(trgmask)+strlen(trg))*2+MAX_PATH);
  sprintf(fsrc,srcmask,src);
  sprintf(ftrg,trgmask,trg);
  strcat(ftrg,"\\");
  char *trgplace=strchr(ftrg,0);
  char *srcplace=strrchr(fsrc,'\\')+1;
  WIN32_FIND_DATA fdata;
  HANDLE h=FindFirstFile(fsrc,&fdata);
  if (h==INVALID_HANDLE_VALUE)
	{
	ResMessageBox2(hWnd,IDS_INSTALLCHYBISOUBOR,MB_ICONEXCLAMATION|MB_OK,fsrc);
	return;
	}
  if (h) do
  	{
	if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	  {
	  strcpy(trgplace,fdata.cFileName);
	  strcpy(srcplace,fdata.cFileName);
	  FileSingleCopy(fsrc,ftrg);
	  }
	}
	while (FindNextFile(h,&fdata));
  FindClose(h);
  }

static void DirCreate(const char *srcmask, const char *src)
  {
  char *fsrc=(char *)alloca((strlen(srcmask)+strlen(src))*2+MAX_PATH);
  sprintf(fsrc,srcmask,src);
  CreateDirectory(fsrc,NULL);
  }

static BOOL CommitCopy(HWND hWnd)
  {
  int size=0;
  int i;
  for (i=0;i<str_count(filetocopy);i++) if (filetocopy[i]) size+=strlen(filetocopy[i])+10;
  char *buff=(char *)malloc(size);
  char *ptr=buff;
  char *trg;
  for (i=0;i<str_count(filetocopy);i+=2) 
	if (filetocopy[i])
	  {
	  strcpy(ptr,filetocopy[i]);
	  ptr=strchr(ptr,0)+1;
	  }
  *ptr++=0;
  trg=ptr;
  for (i=1;i<str_count(filetocopy);i+=2) 
	if (filetocopy[i])
	  {
	  strcpy(ptr,filetocopy[i]);
	  ptr=strchr(ptr,0)+1;
	  }
  *ptr++=0;
  release_list(filetocopy);
  filetocopy=NULL;
  SHFILEOPSTRUCT copyop;
  copyop.hwnd=hWnd;
  copyop.wFunc=FO_COPY;
  copyop.pFrom=buff;
  copyop.pTo=trg;
  copyop.fFlags=FOF_MULTIDESTFILES|FOF_NOCONFIRMATION|FOF_NOCONFIRMMKDIR ;
  copyop.fAnyOperationsAborted=TRUE;
  copyop.lpszProgressTitle="copying";  
  if (SHFileOperation(&copyop)!=0) {free(buff);return FALSE;}
  if (copyop.fAnyOperationsAborted==TRUE) {free(buff);return FALSE;}
  free(buff);
  return TRUE;  
  }

HRESULT CreateLink(LPCSTR lpszPathObj, 
    LPSTR lpszPathLink, LPSTR lpszDesc, LPSTR args) 
{ 
    HRESULT hres; 
    IShellLink* psl; 
 
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, 
        CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl); 
    if (SUCCEEDED(hres)) { 
	  char *wkd=strcpy((char*)alloca(strlen(lpszPathObj)+1),lpszPathObj);
	  *strrchr(wkd,'\\')=0;
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the 
        // description. 
        psl->SetPath(lpszPathObj); 
        psl->SetDescription( lpszDesc); 
		psl->SetWorkingDirectory(wkd);
		if (args!=NULL) psl->SetArguments(args);
 
       // Query IShellLink for the IPersistFile interface for saving the 
       // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, 
            (void **)&ppf); 
 
        if (SUCCEEDED(hres)) { 
            wchar_t wsz[MAX_PATH]; 
 
            // Ensure that the string is ANSI. 
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, 
                wsz, MAX_PATH); 
 
            // Save the link by calling IPersistFile::Save. 
            hres = ppf->Save(wsz, TRUE); 
            ppf->Release(); 
        } 
        psl->Release(); 
    } 
    return hres; 
} 



static void CreateLinkSkeldal(char *buff,char *slozka)
  {
  CoInitialize(NULL);
  GetModuleFileName(NULL,buff,MAX_PATH*4);
  char *c=strrchr(buff,'\\');
  strcat(slozka,c);
  SHGetSpecialFolderPath(NULL,buff,CSIDL_DESKTOPDIRECTORY,TRUE);
  strcat(buff,"\\");
  c=strchr(buff,0);
  LoadString(GetModuleHandle(NULL),IDS_WINTITLE,c,50);
  strcat(c,".lnk");
  CreateLink(slozka,buff,"",NULL);
  }

BOOL RunInstall(HWND hWnd)
  {
  char slozka[MAX_PATH*4];
  char cd[MAX_PATH*4];
  char mypath[MAX_PATH*4];
  TSTR_LIST ini;
  char *c;

  if (ResMessageBox(hWnd,IDS_INSTALLTEXT1,MB_OKCANCEL)==IDCANCEL) return FALSE;
  if (VyberCD(hWnd,cd)==false) return FALSE;
  if (ResMessageBox(hWnd,IDS_INSTALLTEXT2,MB_OKCANCEL)==IDCANCEL) return FALSE;
opakuj1:
  if (VyberSlozku(hWnd,slozka)==false) return FALSE;
  c=strrchr(slozka,'\\');
  if (c==NULL) goto opakuj1;
  c++;
  if (strstr(slozka,c)<c) c[-1]=0;
  if (SetCurrentDirectory(slozka)==FALSE)
	{	
	if (ResMessageBox2(hWnd, IDS_CREATEDIR, MB_YESNO|MB_ICONQUESTION,slozka)==IDNO) goto opakuj1;
	if (CreateDirectory(slozka,NULL)==FALSE)
	  {
	  ResMessageBox2(hWnd,IDS_NELZEVYTVORITSLOZKU,MB_ICONSTOP|MB_OK,slozka);
	  goto opakuj1;
	  }
	if (SetCurrentDirectory(slozka)==FALSE) goto opakuj1;
	}
  c=strrchr(cd,'\\');
  *c=0;
  ini=create_list(1);
  GetModuleFileName(NULL,mypath,sizeof(mypath));  
  FileCopy(hWnd,"%s","%s",mypath,slozka);
  if (ResMessageBox(hWnd,IDS_INSTALLHDD,MB_YESNO|MB_ICONQUESTION)==IDNO)
	{
	sprintf(mypath,"CESTA_MAPY %s\\maps\\",cd);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_MUSIC %s\\music\\",cd);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_VIDEO %s\\video\\",cd);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_DATA %s\\",cd);str_add(&ini,mypath);
	}
  else
	{
    FileCopy(hWnd,"%s\\skeldal.ddl","%s",cd,slozka);
    FileCopy(hWnd,"%s\\popisy.enc","%s",cd,slozka);
    FileCopy(hWnd,"%s\\titulky.enc","%s",cd,slozka);
    FileCopy(hWnd,"%s\\endtext.enc","%s",cd,slozka);
    FileCopy(hWnd,"%s\\maps\\*.map","%s\\maps",cd,slozka);
    FileCopy(hWnd,"%s\\maps\\*.enc","%s\\maps",cd,slozka);
    FileCopy(hWnd,"%s\\maps\\*.dat","%s\\maps",cd,slozka);
    FileCopy(hWnd,"%s\\music\\*.mus","%s\\music",cd,slozka);
    FileCopy(hWnd,"%s\\video\\*.*","%s\\video",cd,slozka);
	sprintf(mypath,"CESTA_MAPY %s\\maps\\",slozka);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_MUSIC %s\\music\\",slozka);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_VIDEO %s\\video\\",slozka);str_add(&ini,mypath);
	sprintf(mypath,"CESTA_DATA %s\\",slozka);str_add(&ini,mypath);
	}
  EnableWindow(hWnd,FALSE);
  BOOL suces=CommitCopy(hWnd);
  EnableWindow(hWnd,TRUE);
  if (suces==FALSE) return FALSE;
  DirCreate("%s\\savegame",slozka);
  DirCreate("%s\\temp",slozka);
  sprintf(mypath,"CESTA_POZICE %s\\savegame\\",slozka);str_add(&ini,mypath);
  sprintf(mypath,"CESTA_TEMPY %s\\temp\\",slozka);str_add(&ini,mypath);
  sprintf(mypath,"CESTA_CD %s\\",cd);str_add(&ini,mypath);
  str_add(&ini,"vmode 0");
  str_add(&ini,"sound_device 9 0 0 0");
  str_add(&ini,"SOUND_MIXFREQ 44100");
  str_add(&ini,"default_map lespred.map");
  save_config(ini,SKELDALINI);
  release_list(ini);
  CreateLinkSkeldal(mypath,slozka);
  ResMessageBox(hWnd,IDS_INSTALLEDGAME,MB_OK|MB_ICONINFORMATION);
  return TRUE;
  }