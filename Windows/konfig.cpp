#include <skeldal_win.h>
#include <d3d9.h>
#include <windowsx.h>
#include <commctrl.h>
#include <malloc.h>
#include <stdio.h>
#include "resource.h"
#include "konfig.h"
#include "../game/extras.h"
#include "richview.h"
#include <shlobj.h>

extern "C"
  {
#include <strlite.h>
#include <inicfg.h>
  }

static HWND handles[5];
static HWND konfigHelp;

#define CFGCHEATS 4
#define CFGEXTRAS 3
#define CFGSOUND 1
#define CFGVIDEO 2
#define CFGGENERAL 0

static LRESULT NullProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  return 0;
  }

static HANDLE StartHelp(HWND hDlg, const char *process,const char *helppathname)
  {
  SHELLEXECUTEINFO nfo;

  memset(&nfo,0,sizeof(nfo));
  nfo.cbSize=sizeof(nfo);
  nfo.fMask=SEE_MASK_DOENVSUBST|SEE_MASK_NOCLOSEPROCESS;
  nfo.hwnd=hDlg;
  nfo.lpFile=process;
  nfo.lpParameters=helppathname;
  nfo.nShow=SW_NORMAL;  
  if (ShellExecuteEx(&nfo)==0) return NULL;
  return nfo.hProcess;
  }

static void ShowHelp(HWND hDlg, char *helpname)
  {
  SetCursor(LoadCursor(NULL,IDC_WAIT));
  HINSTANCE hInstance=GetModuleHandle(NULL);
  HRSRC rscr=FindResource(hInstance,helpname,RT_HTML);
  HGLOBAL hglob=LoadResource(hInstance,rscr);
  void *ptr=LockResource(hglob);
  DWORD sz=SizeofResource(hInstance,rscr);
  DWORD wrt;
  char buff[MAX_PATH*2];
  GetTempPath(sizeof(buff),buff);
  strcat(buff,helpname);
  HANDLE file=CreateFile(buff,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  WriteFile(file,ptr,sz,&wrt,NULL);
  CloseHandle(file);
  HANDLE hlp=StartHelp(hDlg,"hh",buff);
  if (hlp==NULL) hlp=StartHelp(hDlg,"iexplore",buff);
  if (hlp==NULL) {hlp=StartHelp(hDlg,buff,NULL);}
  for (int i=0;i<10;i++) {WaitForInputIdle(hlp,10000);  Sleep(10);}
  while (DeleteFile(buff)==FALSE) 
	{WaitForInputIdle(hlp,10000);  Sleep(10);}
  CloseHandle(hlp);
  }

static bool ModeWarning()
  {
  DEVMODE devmode;
  devmode.dmSize=sizeof(DEVMODE);
  EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
  return devmode.dmBitsPerPel!=16;
  }

int ResMessageBox(HWND hWnd, UINT idc, UINT flags)
  {
  char buff1[1024],buff2[256];
  HINSTANCE hInst=GetModuleHandle(NULL);
  LoadString(hInst,idc,buff1,sizeof(buff1));
  LoadString(hInst,IDS_WINTITLE,buff2,sizeof(buff2));
  return MessageBox(hWnd,buff1,buff2,flags);
  }

int ResMessageBox2(HWND hWnd, UINT idc, UINT flags,...)
  {
  char buff1[1024],buff2[256],buff3[2048];
  va_list args;
  va_start(args,flags);
  HINSTANCE hInst=GetModuleHandle(NULL);
  LoadString(hInst,idc,buff1,sizeof(buff1));
  LoadString(hInst,IDS_WINTITLE,buff2,sizeof(buff2));
  _vsnprintf(buff3,sizeof(buff3),buff1,args);
  return MessageBox(hWnd,buff3,buff2,flags);
  }


  
#include "WAPlayer.h"
static WAPlayer GWinAmpPlayer;

static bool FillListOfPlugins(WAPlayer &player, WAInputPlugin &inplug, void *context)
{
  HWND list=(HWND)context;
  int i=ListBox_AddString(list,inplug.GetDescription());
  ListBox_SetItemData(list,i,(LPARAM)&inplug);
  return true;
}

static void UpdateListOfWinampPlugins(HWND okno)
{
  int len=GetWindowTextLength(GetDlgItem(okno,IDC_CESTANAPLUGINY));
  char *path=(char *)alloca(len+1);
  GetWindowText(GetDlgItem(okno,IDC_CESTANAPLUGINY),path,len+1);
  GWinAmpPlayer.LoadPlugins(path);
  ListBox_ResetContent(GetDlgItem(okno,IDC_PLUGLIST));
  GWinAmpPlayer.EnumPlugins(FillListOfPlugins,(void *)GetDlgItem(okno,IDC_PLUGLIST));
}

static int WINAPI PosBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
  {
  const char *curpath=(const char *)lpData;  
  if (uMsg == BFFM_INITIALIZED)
    {
      if (curpath && curpath[0])
      {
        ::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)((LPCSTR)curpath));
        ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)((LPCSTR)curpath));
      }
    }
    else if (uMsg == BFFM_SELCHANGED)
      {
      char buff[_MAX_PATH];
      if (SHGetPathFromIDList((LPITEMIDLIST)lParam,buff))
        {        
        ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)buff);
        }
      }
  return 0;
  };

static bool PathBrowser(HWND hWnd, char *path /* MAX_PATH size */)
{
  BROWSEINFO brw;
  memset(&brw,0,sizeof(brw));
  brw.hwndOwner=hWnd;
  brw.pidlRoot=NULL;
  brw.pszDisplayName=path;
  brw.lParam=(LPARAM)path;	
#ifdef BIF_USENEWUI
  brw.ulFlags= BIF_RETURNONLYFSDIRS |BIF_STATUSTEXT|BIF_USENEWUI ; 	
#else
  brw.ulFlags= BIF_RETURNONLYFSDIRS |BIF_STATUSTEXT ; 	
#endif
  brw.lpfn = (BFFCALLBACK)(PosBrowseCallbackProc);
  LPITEMIDLIST il=SHBrowseForFolder( &brw );
  if (il==NULL) return false;
  SHGetPathFromIDList(il,path);
  IMalloc *shmalloc;
  SHGetMalloc(&shmalloc);
  shmalloc->Free(il);
  if (path[0]==0) return false;
  return true;
}

static void BrowseCestaNaPluginy(HWND main, HWND control)
{
  int len=GetWindowTextLength(control);
  len+=MAX_PATH;
  char *path=(char *)alloca(len+1);
  GetWindowText(control,path,len+1);
  if (PathBrowser(main,path))
  {
    SetWindowText(control,path);
    UpdateListOfWinampPlugins(main);
  }
}

static void OnPlugVlastnosti(HWND hDlg)
{
  HWND list=GetDlgItem(hDlg,IDC_PLUGLIST);
  int cursel=ListBox_GetCurSel(list);
  if (cursel<0) 
  {
    char buff[256];
    LoadString((HINSTANCE)GetWindowLong(hDlg,GWL_HINSTANCE),IDS_NICVYBRANO,buff,256);
    MessageBox(hDlg,buff,0,MB_OK);
  }
  else
  {
    WAInputPlugin *p=(WAInputPlugin *)ListBox_GetItemData(list,cursel);
    p->UIConfig(hDlg);
  }

}

static LRESULT SubDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  switch (msg)
	{
	case WM_NOTIFY:
	  {
      int idCtrl = (int) wParam; 
	  LPNMHDR pnmh = (LPNMHDR) lParam; 
	  switch (idCtrl)
		{
		case IDC_GAMESPEED:
		  SetDlgItemInt(hDlg,IDC_GAMESPEEDINFO,SendDlgItemMessage(hDlg,IDC_GAMESPEED,TBM_GETPOS,0,0),FALSE);break;
		default:return 0;
		}
	  }
	case WM_COMMAND:
	  {
	  BOOL tmp;
	  int idCtrl=LOWORD(wParam);
	  switch (idCtrl)
		{
		case IDC_WINDOWED:
		  tmp=IsDlgButtonChecked(hDlg,idCtrl)==BST_CHECKED;
		  if (tmp && ModeWarning())
			ResMessageBox(hDlg,IDS_SCREENBPPWARNING,MB_OK|MB_ICONINFORMATION);
		  EnableWindow(GetDlgItem(hDlg,IDC_ZOOM1),tmp);
		  EnableWindow(GetDlgItem(hDlg,IDC_ZOOM2),tmp);
		  EnableWindow(GetDlgItem(hDlg,IDC_ZOOM3),tmp);
		  break;
		case IDC_GAMESPEEDDEFAULT: 
				SendDlgItemMessage(hDlg,IDC_GAMESPEED,TBM_SETPOS,1,20-6);
				SendDlgItemMessage(hDlg,IDC_BATTLEACC,TBM_SETPOS,1,0);break;
		case IDC_NOHUNGRY:
		case IDC_DEBUGCONSOLE:
		  if (IsDlgButtonChecked(hDlg,idCtrl)==BST_CHECKED)
			{
			if (ResMessageBox(hDlg,IDS_CHEATWARNING1,MB_YESNO|MB_ICONQUESTION)!=IDYES ||
			    ResMessageBox(hDlg,IDS_CHEATWARNING2,MB_OKCANCEL|MB_ICONINFORMATION)!=IDCANCEL ||
  				ResMessageBox(hDlg,IDS_CHEATWARNING3,MB_RETRYCANCEL|MB_ICONEXCLAMATION)!=IDRETRY ||
    			ResMessageBox(hDlg,IDS_CHEATWARNING4,MB_ABORTRETRYIGNORE|MB_ICONSTOP)!=IDIGNORE)			  
  				CheckDlgButton(hDlg,idCtrl,BST_UNCHECKED);
  			else
				ResMessageBox(hDlg,IDS_CHEATWARNING5,MB_OK|MB_ICONEXCLAMATION);
			}

		  break;

        case IDC_CESTANAPLUGINY:
          BrowseCestaNaPluginy(hDlg,GetDlgItem(hDlg,IDC_CESTANAPLUGINY));
          UpdateListOfWinampPlugins(hDlg);
          break;
        case IDC_PLUGVLASTNOSTI:
          OnPlugVlastnosti(hDlg);
          break;
        case IDC_PLUGLIST:
          if (HIWORD(wParam)==LBN_DBLCLK) OnPlugVlastnosti(hDlg);
          break;
        case IDC_PLUGINYNAPOVEDA:
          {
		  if (konfigHelp) DestroyWindow(konfigHelp);
		  konfigHelp=(HWND)ShowRichView(hDlg,MAKEINTRESOURCE(IDR_WINAMPPLUGS),"RTF",false);		  
		  RECT rc1,rc2;
		  SystemParametersInfo(SPI_GETWORKAREA,0,&rc1,0);
		  GetWindowRect(konfigHelp,&rc2);		  
		  SetWindowPos(konfigHelp,NULL,rc1.right-rc2.right+rc2.left,0,0,0,SWP_NOSIZE|SWP_NOZORDER);
		  break;
          }
		case IDC_EXTRASHELP:
		  {		  
		  if (konfigHelp) DestroyWindow(konfigHelp);
		  konfigHelp=(HWND)ShowRichView(hDlg,MAKEINTRESOURCE(IDR_EXTRAS),"RTF",false);		  
		  RECT rc1,rc2;
		  SystemParametersInfo(SPI_GETWORKAREA,0,&rc1,0);
		  GetWindowRect(konfigHelp,&rc2);		  
		  SetWindowPos(konfigHelp,NULL,rc1.right-rc2.right+rc2.left,0,0,0,SWP_NOSIZE|SWP_NOZORDER);
		  break;
		  }
		default:return 0;
		}
	  }
	case WM_MOUSEMOVE:
	  {
	  if (IsWindowVisible(handles[CFGCHEATS]))
		{
		EnableWindow(GetDlgItem(handles[CFGCHEATS],IDC_NOHUNGRY),FALSE);
		EnableWindow(GetDlgItem(handles[CFGCHEATS],IDC_DEBUGCONSOLE),FALSE);
		}
	  return 0;
	  }
	default: return 0;
	}
  return 1;
  }



static void LoadSubDialog(HWND hParent, int pos, int uid, DLGPROC dlgproc)
  {
  RECT rc;
  handles[pos]=CreateDialog(GetModuleHandle(NULL),MAKEINTRESOURCE(uid),hParent,dlgproc);
  ShowWindow(handles[pos],SW_HIDE);
  GetWindowRect(GetDlgItem(hParent,IDC_CLIENTDESK),&rc);
  ScreenToClient(hParent,(POINT *)&rc.left);
  ScreenToClient(hParent,(POINT *)&rc.right);
  MoveWindow(handles[pos],rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,FALSE);
  }

static int GetCountOfMonitors()
{
  IDirect3D9 *dx=Direct3DCreate9(D3D_SDK_VERSION);
  if (dx==NULL) return 1;
  int num=dx->GetAdapterCount();
  dx->Release();
  return num;
}

static void SetupDialog()
  {
  int num;
  TSTR_LIST lst=read_config(SKELDALINI);
  SendDlgItemMessage(handles[CFGGENERAL],IDC_GAMESPEED,TBM_SETRANGE,1,MAKELONG(1, 20)); 	
  SendDlgItemMessage(handles[CFGGENERAL],IDC_BATTLEACC,TBM_SETRANGE,1,MAKELONG(0, 5)); 	
  SendDlgItemMessage(handles[CFGSOUND],IDC_SNDMUSIC,TBM_SETRANGE,1,MAKELONG(0,127)); 	
  SendDlgItemMessage(handles[CFGSOUND],IDC_SNDFX,TBM_SETRANGE,1,MAKELONG(0,255)); 	
  CheckDlgButton(handles[CFGGENERAL],IDC_WINDOWED,(get_num_field(lst,"WINDOWED",&num)==0 && num==1)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(handles[CFGGENERAL],IDC_PRELOAD,(get_num_field(lst,"PRELOAD",&num)==0 && num==1)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(handles[CFGGENERAL],IDC_AUTOSAVE,(get_num_field(lst,"AUTOSAVE",&num)==0 && num==1)?BST_CHECKED:BST_UNCHECKED);
  num=6;get_num_field(lst,"GAME_SPEED",&num);
  num=20-num;
  SendDlgItemMessage(handles[CFGGENERAL],IDC_GAMESPEED,TBM_SETPOS,1,num);
  num=0;get_num_field(lst,"BATTLE_ACCEL",&num);
  SendDlgItemMessage(handles[CFGGENERAL],IDC_BATTLEACC,TBM_SETPOS,1,num);
  SetDlgItemInt(handles[CFGGENERAL],IDC_GAMESPEEDINFO,num,FALSE);
  num=44100;get_num_field(lst,"SOUND_MIXFREQ",&num);
  SetDlgItemInt(handles[CFGSOUND],IDC_MIXFREQ,num,FALSE);
  num=127;get_num_field(lst,"MUSIC_VOLUME",&num);
  SendDlgItemMessage(handles[CFGSOUND],IDC_SNDMUSIC,TBM_SETPOS,1,num);
  num=255;get_num_field(lst,"SOUND_VOLUME",&num);
  SendDlgItemMessage(handles[CFGSOUND],IDC_SNDFX,TBM_SETPOS,1,num);
  CheckDlgButton(handles[CFGVIDEO],IDC_SKIPINTRO,(get_num_field(lst,"SKIP_INTRO",&num)==0 && num==1)?BST_CHECKED:BST_UNCHECKED);
  CheckDlgButton(handles[CFGVIDEO],IDC_FULLRESVIDEO,(get_num_field(lst,"FULLRESVIDEO",&num)==0 && num==1)?BST_CHECKED:BST_UNCHECKED);
  num=1;get_num_field(lst,"WINDOWEDZOOM",&num);
  CheckRadioButton(handles[CFGGENERAL],IDC_ZOOM1,IDC_ZOOM3,IDC_ZOOM1+num-1);
  num=0;get_num_field(lst,"MONITOR",&num);
  CheckRadioButton(handles[CFGGENERAL],IDC_MON1,IDC_MON3,IDC_MON1+num);
  const char *cestaNaPluginy=get_text_field(lst,"CESTA_PLUGINS");
  if (cestaNaPluginy==0)
  {
    char *cesta=AutodetectWinAmp();
    SetDlgItemText(handles[CFGSOUND],IDC_CESTANAPLUGINY,cesta);
    free(cesta);
  }
  else
    SetDlgItemText(handles[CFGSOUND],IDC_CESTANAPLUGINY,cestaNaPluginy);
  UpdateListOfWinampPlugins(handles[CFGSOUND]);
  int moncnt=GetCountOfMonitors();
  if (moncnt<3) EnableWindow(GetDlgItem(handles[CFGGENERAL],IDC_MON3),FALSE);
  if (moncnt<2) EnableWindow(GetDlgItem(handles[CFGGENERAL],IDC_MON2),FALSE);
  num=0;get_num_field(lst,"EXTRAS",&num);
  for (int i=0;i<16;i++)
	{
	int p=1<<i;	
	if (num & p) 
	  if (p==EX_NOHUNGRY) CheckDlgButton(handles[CFGCHEATS],IDC_NOHUNGRY,BST_CHECKED);
	  else CheckDlgButton(handles[CFGEXTRAS],2000+i,BST_CHECKED);
	}
  num=0;get_num_field(lst,"DEBUG",&num);
  if (num) CheckDlgButton(handles[CFGCHEATS],IDC_DEBUGCONSOLE,BST_CHECKED);
  if (num>1) SetDlgItemInt(handles[CFGCHEATS],IDC_CHEATPASSWORD,num,FALSE);
  release_list(lst);
  }

static int RadioButtonChecked(HWND hWnd, int min, int max)
{
  for (int i=min;i<=max;i++)
  {
	if (IsDlgButtonChecked(hWnd,i)==BST_CHECKED) return i-min;
  }
  return -1;
}

static void SaveKonfig(HWND hDlg)
  {
  TSTR_LIST lst=read_config(SKELDALINI);
  add_field_num(&lst,"WINDOWED",IsDlgButtonChecked(handles[CFGGENERAL],IDC_WINDOWED)==BST_CHECKED);
  add_field_num(&lst,"PRELOAD",IsDlgButtonChecked(handles[CFGGENERAL],IDC_PRELOAD)==BST_CHECKED);
  add_field_num(&lst,"AUTOSAVE",IsDlgButtonChecked(handles[CFGGENERAL],IDC_AUTOSAVE)==BST_CHECKED);
  add_field_num(&lst,"SKIP_INTRO",IsDlgButtonChecked(handles[CFGVIDEO],IDC_SKIPINTRO)==BST_CHECKED);
  add_field_num(&lst,"FULLRESVIDEO",IsDlgButtonChecked(handles[CFGVIDEO],IDC_FULLRESVIDEO)==BST_CHECKED);
  add_field_num(&lst,"DEBUG",IsDlgButtonChecked(handles[CFGCHEATS],IDC_DEBUGCONSOLE)==BST_CHECKED);
  int mixfreq=GetDlgItemInt(handles[CFGSOUND],IDC_MIXFREQ,NULL,FALSE);
  if (mixfreq<8000 || mixfreq>44100) 
	ResMessageBox(hDlg,IDS_MIXFREQINVALID,MB_OK|MB_ICONEXCLAMATION);
  else
	add_field_num(&lst,"SOUND_MIXFREQ",mixfreq);
  add_field_num(&lst,"MUSIC_VOLUME",SendDlgItemMessage(handles[CFGSOUND],IDC_SNDMUSIC,TBM_GETPOS,0,0));
  add_field_num(&lst,"SOUND_VOLUME",SendDlgItemMessage(handles[CFGSOUND],IDC_SNDFX,TBM_GETPOS,0,0));
  add_field_num(&lst,"GAME_SPEED",20-SendDlgItemMessage(handles[CFGGENERAL],IDC_GAMESPEED,TBM_GETPOS,0,0));
  add_field_num(&lst,"BATTLE_ACCEL",SendDlgItemMessage(handles[CFGGENERAL],IDC_BATTLEACC,TBM_GETPOS,0,0));
  add_field_num(&lst,"WINDOWEDZOOM",RadioButtonChecked(handles[CFGGENERAL],IDC_ZOOM1,IDC_ZOOM3)+1);
  add_field_num(&lst,"MONITOR",RadioButtonChecked(handles[CFGGENERAL],IDC_MON1,IDC_MON3));
  int acc=0;
  for (int i=0;i<16;i++)
	{
	int p=1<<i;
	if (p==EX_NOHUNGRY && IsDlgButtonChecked(handles[CFGCHEATS],IDC_NOHUNGRY)==BST_CHECKED) acc+=p;
	else if (IsDlgButtonChecked(handles[CFGEXTRAS],2000+i)==BST_CHECKED) acc+=p;
	}
  add_field_num(&lst,"EXTRAS",acc);
  acc=GetDlgItemInt(handles[CFGCHEATS],IDC_CHEATPASSWORD,NULL,FALSE);
  if (acc>1) add_field_num(&lst,"DEBUG",acc);
  acc=GetWindowTextLength(GetDlgItem(handles[CFGSOUND],IDC_CESTANAPLUGINY));
  char *buff=(char *)alloca(acc+1);
  GetWindowText(GetDlgItem(handles[CFGSOUND],IDC_CESTANAPLUGINY),buff,acc+1);
  add_field_txt(&lst,"CESTA_PLUGINS",buff);
  save_config(lst,SKELDALINI);  
  }

static LRESULT KonfigProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  switch (msg)
    {
    case WM_INITDIALOG:
      LoadSubDialog(hDlg,CFGGENERAL,IDD_CFGGENERAL,(DLGPROC)SubDlgProc);
      LoadSubDialog(hDlg,CFGSOUND,IDD_CFGSOUND,(DLGPROC)SubDlgProc);
      LoadSubDialog(hDlg,CFGVIDEO,IDD_CFGVIDEO,(DLGPROC)NullProc);
      LoadSubDialog(hDlg,CFGEXTRAS,IDD_CFGEXTRAS,(DLGPROC)SubDlgProc);
      LoadSubDialog(hDlg,CFGCHEATS,IDD_CFGCHEATS,(DLGPROC)SubDlgProc);
        {
        for (int i=0;i<5;i++)
          {
          TCITEM item;
          char buff[256];
          item.mask=TCIF_TEXT;
          LoadString(GetModuleHandle(NULL),IDS_CFGGENERAL+i,buff,sizeof(buff));
          item.pszText=buff;
          SendDlgItemMessage(hDlg,IDC_TAB,TCM_INSERTITEM,i,(LPARAM)&item);
          }
        ShowWindow(handles[0],SW_SHOW);
		SetupDialog();
        }
	  konfigHelp=NULL;
      break;

    case WM_NOTIFY:
      {
      LPNMHDR lpnmhdr = (LPNMHDR) lParam;
      if (lpnmhdr->idFrom==IDC_TAB && lpnmhdr->code==TCN_SELCHANGE)
        {
        int cursel=TabCtrl_GetCurSel(GetDlgItem(hDlg,IDC_TAB));
        for (int i=0;i<5;i++)          
          ShowWindow(handles[i],i==cursel?SW_SHOW:SW_HIDE);          
		if (cursel==CFGCHEATS)
		  {
		  EnableWindow(GetDlgItem(handles[cursel],IDC_NOHUNGRY),TRUE);
		  EnableWindow(GetDlgItem(handles[cursel],IDC_DEBUGCONSOLE),TRUE);
		  }
        }
      break;
      }
    case WM_COMMAND:
      {
      switch (LOWORD(wParam))
        {
        case IDCANCEL: EndDialog(hDlg,0);break;
		case IDOK: SaveKonfig(hDlg);EndDialog(hDlg,1);break;
        }
      return 1;
      }
	case WM_DESTROY:
	  konfigHelp=NULL;
	  break;
	case WM_MOUSEMOVE:
	  {
	  if (IsWindowVisible(handles[CFGCHEATS]))
		{
		EnableWindow(GetDlgItem(handles[CFGCHEATS],IDC_NOHUNGRY),FALSE);
		EnableWindow(GetDlgItem(handles[CFGCHEATS],IDC_DEBUGCONSOLE),FALSE);
		}
	  return 0;
	  }
    default: return 0;
    }
  return 1;
  }


void OpenKonfig(HWND hParent)
  {
  DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CFGTABDIALOG),hParent,(DLGPROC)KonfigProc,NULL);
  GWinAmpPlayer.ClearList();
  }