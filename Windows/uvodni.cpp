#include <skeldal_win.h>
#include <windowsx.h>
#include <commctrl.h>
#include "uvodni.h"
#include "resource.h"
#include "konfig.h"
#include "install.h"
#include <io.h>
#include <SHLOBJ.H>
#include "..\game\version.h"


#define UVODNIOKNOCLASS "Uvodni okno hry Brany Skeldalu@bredysoft"
#define SELPOSX 0
#define SELPOSY 279

LRESULT UvodniWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void RegistrujTriduOkna()
  {
  WNDCLASSEX cls;
  memset(&cls,0,sizeof(cls));
  cls.cbSize=sizeof(cls);
  cls.hCursor=LoadCursor(NULL,IDC_ARROW);
  cls.hInstance=GetModuleHandle(NULL);
  cls.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAINICON));
  cls.lpfnWndProc=(WNDPROC)UvodniWinProc;
  cls.lpszClassName=UVODNIOKNOCLASS;
  if (RegisterClassEx(&cls)==NULL) 
    {
    MessageBox(NULL,"Nelze registrovat tridu okna",NULL,MB_OK|MB_ICONSTOP);
    exit(-1);
    }
  }

static HBITMAP pozadi;
static HBITMAP selpozadi;
static HDC pozadi_dc;
static HDC selpozadi_dc;
static HBITMAP pozadi_dc_old;
static HBITMAP selpozadi_dc_old;
static int exitstatus=0;

static int GetSelection(HWND hWnd, DWORD pt,RECT *rcout)
  {
  POINT ptt;
  BITMAP bmpi;
  ptt.x=GET_X_LPARAM(pt);
  ptt.y=GET_Y_LPARAM(pt);
  ScreenToClient(hWnd,&ptt);
  GetObject(selpozadi,sizeof(bmpi),&bmpi);
  RECT rc;
  rc.left=SELPOSX;
  rc.top=SELPOSY;
  rc.right=rc.left+bmpi.bmWidth;
  rc.bottom=rc.top+bmpi.bmHeight;
  for (int i=0;i<3;i++)
    {
    RECT rrc=rc;
    rrc.top=rc.top+((rc.bottom-rc.top)*i)/3;
    rrc.bottom=rc.top+((rc.bottom-rc.top)*(i+1))/3;
    if (PtInRect(&rrc,ptt))
      {
      if (rcout) *rcout=rrc;
      return i;
      }
    }
  return -1;
  }

static void UkazSelekci(HWND hWnd, HDC hDc, DWORD pt)
  {
  RECT rc;
  int res=GetSelection(hWnd,pt,&rc);
  if (res==-1) return;
  BitBlt(hDc,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,selpozadi_dc,rc.left-SELPOSX,rc.top-SELPOSY,SRCCOPY);
  }

static HFONT vfont;

static void ShowVersion(HDC dc)
{
  HFONT fnt=(HFONT )SelectObject(dc,vfont);
  char ver[200]="Version: "VERSION;
  int len=strlen(ver);
  SIZE sz;
  ::GetTextExtentPoint(dc,ver,len,&sz);

  SetTextColor(dc,RGB(255,255,255));
  SetBkMode(dc,TRANSPARENT);
  TextOut(dc,630-sz.cx,450-sz.cy,ver,len);  
  SelectObject(dc,fnt);
}

static LRESULT UvodniWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  static int lastsel;
  static RECT lastselrect;
  switch (msg)
    {
    case WM_CREATE:
      {
      HDC wdc=GetDC(hWnd);
      pozadi=LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_UVODNIPOZADI));
      selpozadi=LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_UVODNISELECT));
      pozadi_dc=CreateCompatibleDC(wdc);
      selpozadi_dc=CreateCompatibleDC(wdc);
      ReleaseDC(hWnd,wdc);
      pozadi_dc_old=(HBITMAP)SelectObject(pozadi_dc,pozadi);
      selpozadi_dc_old=(HBITMAP)SelectObject(selpozadi_dc,selpozadi);
      lastsel=-1;
	  if (_access(SKELDALINI,06)!=0) PostMessage(hWnd,WM_APP,0,0);
	  vfont=CreateFont(15,0,0,0,0,0,0,0,0,0,0,0,0,"Arial");
      return 1;
      }
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC wdc=BeginPaint(hWnd,&ps);
      BitBlt(wdc,0,0,640,480,pozadi_dc,0,0,SRCCOPY);
	  ShowVersion(wdc);
      UkazSelekci(hWnd,wdc,GetMessagePos());
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_SETCURSOR:
      if (lastsel!=-1)        
        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32649)));        
      else
        SetCursor(LoadCursor(NULL, IDC_ARROW));
      break;
    case WM_MOUSEMOVE:
      {
      RECT rc;
      int p=GetSelection(hWnd,GetMessagePos(),&rc);
      if (p!=lastsel)
        {
        InvalidateRect(hWnd,&rc,FALSE);
        InvalidateRect(hWnd,&lastselrect,FALSE);
        lastsel=p;
        lastselrect=rc;
        }
      }
      break;
    case WM_CLOSE:
      exitstatus=0;
      DestroyWindow(hWnd);
      break;
    case WM_DESTROY:
      SelectObject(pozadi_dc,pozadi_dc_old);
      SelectObject(selpozadi_dc,selpozadi_dc_old);
      DeleteObject(pozadi);
      DeleteObject(selpozadi);
      DeleteDC(pozadi_dc);
      DeleteDC(selpozadi_dc);
      PostQuitMessage(exitstatus);
	  DeleteObject(vfont);
      break;    
    case WM_LBUTTONUP:
      {
      int p=GetSelection(hWnd,GetMessagePos(),NULL);
      if (p!=-1)
        {
        switch (p)
          {
          case 2: exitstatus=0; DestroyWindow(hWnd);break;
          case 0: exitstatus=1; DestroyWindow(hWnd);break;
          case 1: OpenKonfig(hWnd);break;
//          case 0: RunInstall(hWnd);break;
          }        
        }
      break;
      }
	case WM_APP:
	  if (RunInstall(hWnd)==FALSE) {exitstatus=0;DestroyWindow(hWnd);}
	  break;
    default:return DefWindowProc(hWnd,msg,wParam,lParam);
    }
  return 1;
  }

extern "C"
  {


char OtevriUvodniOkno()
  {
  RECT rc;

  char buff[256];

  LoadString(GetModuleHandle(NULL),IDS_WINTITLE,buff,sizeof(buff));

  InitCommonControls();

  RegistrujTriduOkna();
  GetClientRect(GetDesktopWindow(),&rc);
  rc.left=(rc.right-640)/2;
  rc.top=(rc.bottom-480)/2;

  AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);

  HWND hOknoWnd=CreateWindow(UVODNIOKNOCLASS,buff,WS_OVERLAPPED|WS_SYSMENU|WS_VISIBLE|WS_MINIMIZEBOX|WS_BORDER,rc.left,rc.top,640,480,NULL,NULL,GetModuleHandle(NULL),NULL);
  MSG msg;  
  while (GetMessage(&msg,0,0,0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
  UnregisterClass(UVODNIOKNOCLASS,GetModuleHandle(NULL));
  return msg.wParam==1;
  }

static void CreateAdvLink(char *advname)
  {  
  char modname[MAX_PATH*4];
  char linkname[MAX_PATH*4];
  GetModuleFileName(NULL,modname,MAX_PATH*4);
  CoInitialize(NULL);
  SHGetSpecialFolderPath(NULL,linkname,CSIDL_DESKTOPDIRECTORY,TRUE);
  strcat(linkname,"\\");
  strcat(linkname,advname);
  strcat(linkname,".lnk");
  CreateLink(modname,linkname,"",advname);
  }


static char selectedAdv[MAX_PATH];
static LRESULT SelectAdventureDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  char temp[MAX_PATH];
  switch (msg)
	{
	case WM_INITDIALOG:
	  {
	  WIN32_FIND_DATA fdata;
	  HANDLE fnd=FindFirstFile("*.adv",&fdata);
	  selectedAdv[0]=0;
	  if (fnd!=INVALID_HANDLE_VALUE)
		{
		HWND lsbx=GetDlgItem(hDlg,IDC_LIST);
		LoadString(GetModuleHandle(NULL),IDS_DEFAULTADV,temp,sizeof(temp));
		ListBox_AddString(lsbx,temp);
		do  ListBox_AddString(lsbx,fdata.cFileName); while (FindNextFile(fnd,&fdata));		
		FindClose(fnd);
	    ListBox_SetCurSel(lsbx,0);
  	    EnableWindow(GetDlgItem(hDlg,IDC_CREATELINK),0);
		}
	  else
		EndDialog(hDlg,IDOK);
	  }
  	  break;
	case WM_DRAWITEM: if (wParam==IDC_LIST)
	  {
	  LPDRAWITEMSTRUCT drawinfo=(LPDRAWITEMSTRUCT)lParam;	  
	  char *c;
	  if (drawinfo->itemState & ODS_SELECTED)
		{
		SetTextColor(drawinfo->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
		SetBkColor(drawinfo->hDC,GetSysColor(COLOR_HIGHLIGHT));
		}
	  else
		{
		SetTextColor(drawinfo->hDC,GetSysColor(COLOR_WINDOWTEXT));
		SetBkColor(drawinfo->hDC,GetSysColor(COLOR_WINDOW));
		}	  
	  ListBox_GetText(drawinfo->hwndItem,drawinfo->itemID,temp);
	  c=strrchr(temp,'.');
	  if (c) *c=0;
	  ExtTextOut(drawinfo->hDC,0,0,ETO_OPAQUE,&drawinfo->rcItem,"",0,NULL);
	  DrawText(drawinfo->hDC,temp,strlen(temp),&drawinfo->rcItem,DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);
	  }
	  break;
	case WM_COMMAND:
	  switch (LOWORD(wParam))
		{
		case IDOK:
		  {
		  HWND lsbx=GetDlgItem(hDlg,IDC_LIST);
		  int cursel=ListBox_GetCurSel(lsbx);
		  if (cursel!=0)			
			{
			ListBox_GetText(lsbx,cursel,selectedAdv);
			if (IsDlgButtonChecked(hDlg,IDC_CREATELINK) & BST_CHECKED)
			  CreateAdvLink(selectedAdv);
			}
		  EndDialog(hDlg,IDOK);
		  }
		  break;
		case IDCANCEL:
		  EndDialog(hDlg,IDCANCEL);
		  break;
		case IDC_LIST:
		  if (HIWORD(wParam)==LBN_DBLCLK) 
			PostMessage(hDlg,WM_COMMAND,IDOK,0);
		  if (HIWORD(wParam)==LBN_SELCHANGE)
			EnableWindow(GetDlgItem(hDlg,IDC_CREATELINK),ListBox_GetCurSel(GetDlgItem(hDlg,IDC_LIST))!=0);
		  break;

		default:return 0;
		}
	  break;
	default: return 0;
	}
  return 1;
  }

char SelectAdventure()
  {
  int ret=DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SELADV), NULL, (DLGPROC)SelectAdventureDlg);
  if (ret==IDCANCEL) exit(0);
  if (selectedAdv[0]==0) return 0;
  return 1;
  }

char *GetSelectedAdventure()
  {
  return selectedAdv;
  }

  }
