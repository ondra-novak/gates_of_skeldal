#include <skeldal_win.h>
#include <resource.h>
#include <richedit.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>

static bool IsModal=false;

static DWORD CALLBACK RtfViewerCallback(
  DWORD dwCookie, // application-defined value
  LPBYTE pbBuff,  // pointer to a buffer
  LONG cb,        // number of bytes to read or write
  LONG *pcb       // pointer to number of bytes transferred
)
  {
  char **rdptr=(char **)dwCookie;
  char *p=*rdptr;
  *pcb=0;
  while (*p && cb)
	{
	*pbBuff=*(LPBYTE)p;
	pbBuff++;
	p++;
	cb--;
	pcb[0]++;
	}
  *rdptr=p;
  return 0;
  }


static LRESULT RtfViewer(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  switch (msg)
	{
	case WM_INITDIALOG:
	  {
	  HWND rtf=GetDlgItem(hWnd,IDC_RICHEDIT);
	  EDITSTREAM edstr;
	  char *rdptr=(char *)lParam;
	  edstr.dwCookie=(DWORD)(&rdptr);
	  edstr.pfnCallback=RtfViewerCallback;
	  SendMessage(rtf,EM_STREAMIN,SF_RTF|SFF_PLAINRTF,(LPARAM)(&edstr));
	  break;
	  }
	case WM_COMMAND:
	  if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL)
		{
		if (IsModal) EndDialog(hWnd,LOWORD(wParam));
		else DestroyWindow(hWnd);
		break;
		}
	  return 0;
	default: return 0;
	}
  return 1;
  }

DWORD ShowRichView(HWND hWnd, const char *resourceName, const char *resourceType, bool modal)
  {

  HMODULE mod;
  mod=GetModuleHandle("RICHED20.DLL");
  if (mod==NULL) mod=LoadLibrary("RICHED20.DLL");
  if (mod==NULL) 
	{
	char buff[256];
	LoadString(GetModuleHandle(NULL),IDS_RICHEDMISSING,buff,sizeof(buff));
	MessageBox(hWnd,buff,NULL,MB_OK|MB_ICONEXCLAMATION);
	return 0;
	}
  HRSRC rsrc=FindResource(GetModuleHandle(NULL),resourceName,resourceType);
  if (rsrc==NULL) return 0;
  HGLOBAL src=LoadResource(GetModuleHandle(NULL),rsrc);
  const char *ptr=(const char *)LockResource(src);
  DWORD res=0;
  if (modal)
	{
	IsModal=true;
	res=DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_RTFVIEWER),hWnd,(DLGPROC)RtfViewer,(LPARAM)ptr);
	IsModal=false;	
	}
  else
	{
	HWND hDlg=CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_RTFVIEWER),hWnd,(DLGPROC)RtfViewer,(LPARAM)ptr);
	ShowWindow(hDlg,SW_SHOW);
	res=(DWORD)hDlg;
	}
  return res;
  }
