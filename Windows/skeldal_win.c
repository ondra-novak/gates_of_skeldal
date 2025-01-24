#include "skeldal_win.h"
#include "debug.h"
#include "resource.h"
#include <devices.h>

void DXMouseTransform(unsigned short *x, unsigned short *y);
void DxLockBuffers(BOOL lock);

HKL english_layout=NULL;

#define WM_EXTRACHAR (WM_APP+100)

#define MAX_KEYQUEUE 16

#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))


static unsigned long keyqueue[MAX_KEYQUEUE];
static unsigned long keyqueuelen=0;
static char wheel_mapping[2]={'Q','I'};
static BOOL noextra;
MS_EVENT win_mouseEvent;
static BOOL ActiveWindow=FALSE;
static DWORD WaitNext=0;

void SetWheelMapping(char up, char down)
{
  wheel_mapping[0]=down;
  wheel_mapping[1]=up;
}

static void SetMouseEvent(MS_EVENT *event,UINT msg, WPARAM wParam,LPARAM lParam)
  {
  event->event_type=0;
  switch (msg)
    {
    case WM_LBUTTONDOWN: event->event_type=0x2;break;
    case WM_LBUTTONUP: event->event_type=0x4;break;
    case WM_RBUTTONDOWN: event->event_type=0x8;break;
    case WM_RBUTTONUP: event->event_type=0x10;break;
    case WM_MBUTTONDOWN: event->event_type=0x20;break;
    case WM_MBUTTONUP: event->event_type=0x40;break;
    case WM_MOUSEMOVE: event->event_type=0x1;break;
    }
  event->x=GET_X_LPARAM(lParam);
  event->y=GET_Y_LPARAM(lParam);
  event->tl1=(wParam & MK_LBUTTON)!=0;
  event->tl3=(wParam & MK_MBUTTON)!=0;
  event->tl2=(wParam & MK_RBUTTON)!=0;
  event->event=1;  
  DXMouseTransform(&event->x,&event->y);  
  }

LRESULT GameMainWindowWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
  if (msg>=WM_MOUSEFIRST && msg<=WM_MOUSELAST && msg!=WM_MOUSEWHEEL)
    {
    SetMouseEvent(&win_mouseEvent,msg,wParam,lParam);
    }
  else switch (msg)
    {
    case WM_CREATE:
       if (english_layout==NULL) english_layout=LoadKeyboardLayout("00000409",KLF_ACTIVATE);
       DSReportWindowCreation(hWnd);
       return TRUE;
    case WM_ENDSESSION:
    case WM_CLOSE:
        DXCloseMode();
        exit(1);
        break;    
    case WM_SETCURSOR: if (LOWORD(lParam)==HTCLIENT)SetCursor(NULL);
          else return DefWindowProc(hWnd,msg,wParam,lParam);break;
          return TRUE;
    case WM_EXTRACHAR: if (noextra) {noextra=FALSE;break; }
    case WM_CHAR:
        if (keyqueuelen<16)
          {
          memmove(keyqueue,keyqueue+1,sizeof(*keyqueue)*keyqueuelen);
          keyqueuelen++;
          keyqueue[0]=MAKEWORD(wParam,HIWORD(lParam));      
          }
        if (msg==WM_CHAR) noextra=TRUE;
        break;
	case WM_MOUSEWHEEL:
	  {
		short delta=HIWORD(wParam);
        if (keyqueuelen<16)
          {
          memmove(keyqueue,keyqueue+1,sizeof(*keyqueue)*keyqueuelen);
          keyqueuelen++;
          keyqueue[0]=MAKEWORD(0,delta<0?wheel_mapping[0]:wheel_mapping[1]);
          }
		break;
	  }
    case WM_ACTIVATE:
      {
        int fActive = LOWORD(wParam);
        ActiveWindow=fActive!=WA_INACTIVE;
        break;
      }
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      BeginPaint(hWnd,&ps);
      DXCopyRects64(0,0,DxGetResX(),DxGetResY());
      EndPaint(hWnd,&ps);
      break;      
      }
	case WM_RELOADMAP:
	  {
		char buff[256];
		int sektor=wParam;
		GlobalGetAtomName((ATOM)lParam,buff,256);
		GlobalDeleteAtom((ATOM)lParam);
		send_message(E_RELOADMAP,buff,sektor);
		break;
	  }

    default: return DefWindowProc(hWnd,msg,wParam,lParam);break;
    }
  return 0;
  }


void CheckMessageQueue()
  {
  MSG msg;
  while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
    {
    TranslateMessage(&msg);
    if (msg.message==WM_KEYDOWN)
      {
      PostMessage(msg.hwnd,WM_EXTRACHAR,0,msg.lParam);
      }
    DispatchMessage(&msg);
    }
  }

void WaitMsgQueue()
  {
  WaitMessage();
  CheckMessageQueue();
  }

unsigned long _bios_keybrd(int mode)
  {
repeat:
  if (keyqueuelen)
    {
    if (mode==_KEYBRD_READY) return 1;
    if (mode==_KEYBRD_READ) {keyqueuelen--;return keyqueue[keyqueuelen];}
    }
  if (mode==_KEYBRD_READ) {WaitMsgQueue(); goto repeat;}
  CheckMessageQueue();  
  return keyqueuelen!=0;
  }

void task_terminating()
  {
  STOP();
  }

void *LoadResourceFont(const char *name)
  {
  HINSTANCE hInst=GetModuleHandle(NULL);
  HRSRC rsrc=FindResource(hInst,name,"SKELDAL_FONT");
  HGLOBAL glb=LoadResource(hInst,rsrc);
  return LockResource(glb);
  }

void *LoadDefaultFont()
  {
  return LoadResourceFont(MAKEINTRESOURCE(IDR_BOLDCZ));
  }


void ShareCPU()
{  
  DWORD curTime=GetTickCount(); 
  DWORD timeout=WaitNext<curTime?0:WaitNext-curTime;
  DWORD res=WAIT_TIMEOUT;
  if (WaitNext==0) WaitNext=GetTickCount();

  DxLockBuffers(FALSE);
  if (timeout) res=MsgWaitForMultipleObjects(0,NULL,FALSE,timeout,QS_ALLINPUT);
  if (res==WAIT_TIMEOUT) WaitNext=WaitNext+10;
  DxLockBuffers(TRUE);
}

char *AutodetectWinAmp()
{
    char *cwd = getcwd(0);
    char *fullpath = (char *)malloc(strlen(cwd)+50);
    sprintf(fullpath,"%s\\Plugins",cwd);
    free(cwd);
    return fullpath;
}

