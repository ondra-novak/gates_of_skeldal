#include <windows.h>
#define DWORD_PTR DWORD *
#include <d3d9.h>
#include <debug.h>
#include "Skeldal_win.h"
#include "resource.h"

#define SKELDALCLASSNAME "BranySkeldalDXWindow"

#define INWINDOW runinwindow

static int dxWindowZoom=1;

static HWND hMainWnd;
static IDirect3D9 *DxHandle;
static IDirect3DDevice9 *DxDevice;
static IDirect3DSurface9 *DxBackBuffer;
static D3DPRESENT_PARAMETERS pparm;

static int initSizeX = 640;
static int initSizeY = 480;

static unsigned short *mainBuffer=NULL;
static unsigned short *secondBuffer=NULL;
static unsigned short *curBuffer=NULL;
static uint32_t main_linelen;

#ifdef _DX_REF
#define DXDEVICE_TYPE D3DDEVTYPE_REF
#else 
#define DXDEVICE_TYPE D3DDEVTYPE_HAL
#endif

static bool dialogs=false;
static bool runinwindow=false;
static int shiftscrx=0,shiftscry=0;

void DXMouseTransform(unsigned short *x, unsigned short *y)
{
  *x=*x*2/dxWindowZoom;
  *y=*y*2/dxWindowZoom;
}

static const char *GetDXResult(HRESULT res)
{
  const char *text;
  switch (res)
  {
  case D3DOK_NOAUTOGEN:text="D3DOK_NOAUTOGEN\r\n\r\nThis is a success code. However, the autogeneration of mipmaps is not supported for this format. This means that resource creation will succeed but the mipmap levels will not be automatically generated.";break;
  case D3DERR_CONFLICTINGRENDERSTATE:text="D3DERR_CONFLICTINGRENDERSTATE\r\n\r\nThe currently set render states cannot be used together.";break;
  case D3DERR_CONFLICTINGTEXTUREFILTER:text="D3DERR_CONFLICTINGTEXTUREFILTER\r\n\r\nThe current texture filters cannot be used together.";break;
  case D3DERR_CONFLICTINGTEXTUREPALETTE:text="D3DERR_CONFLICTINGTEXTUREPALETTE\r\n\r\nThe current textures cannot be used simultaneously.";break;
  case D3DERR_DEVICELOST:text="D3DERR_DEVICELOST\r\n\r\nThe device has been lost but cannot be reset at this time. Therefore, rendering is not possible.";break;
  case D3DERR_DEVICENOTRESET:text="D3DERR_DEVICENOTRESET\r\n\r\nThe device has been lost but can be reset at this time.";break;
  case D3DERR_DRIVERINTERNALERROR:text="D3DERR_DRIVERINTERNALERROR\r\n\r\nInternal driver error. Applications should destroy and recreate the device when receiving this error. For hints on debugging this error, see Driver Internal Errors.";break;
  case D3DERR_DRIVERINVALIDCALL:text="D3DERR_DRIVERINVALIDCALL\r\n\r\nNot used.";break;
  case D3DERR_INVALIDCALL:text="D3DERR_INVALIDCALL\r\n\r\nThe method call is invalid. For example, a method's parameter may not be a valid pointer.";break;
  case D3DERR_INVALIDDEVICE:text="D3DERR_INVALIDDEVICE\r\n\r\nThe requested device type is not valid.";break;
  case D3DERR_MOREDATA:text="D3DERR_MOREDATA\r\n\r\nThere is more data available than the specified buffer size can hold.";break;
  case D3DERR_NOTAVAILABLE:text="D3DERR_NOTAVAILABLE\r\n\r\nZarizeni neni podporovano. Tato chyba vetsinou vznika pri problemu graficke karty a ovladacu. Prosim preinstalujte ovladace ke graficke karte. (\"This device does not support the queried technique.\")";break;
  case D3DERR_NOTFOUND:text="D3DERR_NOTFOUND\r\n\r\nThe requested item was not found.";break;
  case D3DERR_OUTOFVIDEOMEMORY:text="D3DERR_OUTOFVIDEOMEMORY\r\n\r\nDirect3D does not have enough display memory to perform the operation.";break;
  case D3DERR_TOOMANYOPERATIONS:text="D3DERR_TOOMANYOPERATIONS\r\n\r\nThe application is requesting more texture-filtering operations than the device supports.";break;
  case D3DERR_UNSUPPORTEDALPHAARG:text="D3DERR_UNSUPPORTEDALPHAARG\r\n\r\nThe device does not support a specified texture-blending argument for the alpha channel.";break;
  case D3DERR_UNSUPPORTEDALPHAOPERATION:text="D3DERR_UNSUPPORTEDALPHAOPERATION\r\n\r\nThe device does not support a specified texture-blending operation for the alpha channel.";break;
  case D3DERR_UNSUPPORTEDCOLORARG:text="D3DERR_UNSUPPORTEDCOLORARG\r\n\r\nThe device does not support a specified texture-blending argument for color values.";break;
  case D3DERR_UNSUPPORTEDCOLOROPERATION:text="D3DERR_UNSUPPORTEDCOLOROPERATION\r\n\r\nThe device does not support a specified texture-blending operation for color values.";break;
  case D3DERR_UNSUPPORTEDFACTORVALUE:text="D3DERR_UNSUPPORTEDFACTORVALUE\r\n\r\nThe device does not support the specified texture factor value. Not used; provided only to support older drivers.";break;
  case D3DERR_UNSUPPORTEDTEXTUREFILTER:text="D3DERR_UNSUPPORTEDTEXTUREFILTER\r\n\r\nThe device does not support the specified texture filter.";break;
  case D3DERR_WASSTILLDRAWING:text="D3DERR_WASSTILLDRAWING\r\n\r\nThe previous blit operation that is transferring information to or from this surface is incomplete.";break;
  case D3DERR_WRONGTEXTUREFORMAT:text="D3DERR_WRONGTEXTUREFORMAT\r\n\r\nThe pixel format of the texture surface is not valid.";break;
  case E_FAIL:text="E_FAIL\r\n\r\nAn undetermined error occurred inside the Direct3D subsystem.";break;
  case E_INVALIDARG:text="E_INVALIDARG\r\n\r\nAn invalid parameter was passed to the returning function.";break;
//  case E_INVALIDCALL:text="E_INVALIDCALL\r\n\r\nThe method call is invalid. For example, a method's parameter may have an invalid value.";break;
  case E_NOINTERFACE:text="E_NOINTERFACE\r\n\r\nNo object interface is available.";break;
  case E_NOTIMPL:text="E_NOTIMPL\r\n\r\nNot implemented.";break;
  case E_OUTOFMEMORY:text="E_OUTOFMEMORY\r\n\r\nDirect3D could not allocate sufficient memory to complete the call.";break;
  default: text="Neznama chyba DX";break;
  }
  return text;
}

static inline void CheckResult(HRESULT res)
  {
  if (res==0) return;
  char buff[512];
  sprintf(buff,"Chyba pri praci s DirectX: %s\r\nDxResult failed %8X (%d)",GetDXResult(res),res,res&0xFFFF);
  MessageBox(NULL,buff,NULL,MB_OK);
  ExitProcess(res);
  }

extern "C" 
  {

void setvesa_displaystart(int x,int y)
  {
  WINDOWPLACEMENT wp;
  wp.length=sizeof(wp);
  GetWindowPlacement(hMainWnd,&wp);
  wp.ptMaxPosition.x+=x-shiftscrx;
  wp.ptMaxPosition.y+=y-shiftscry;
  wp.rcNormalPosition.left+=x-shiftscrx;
  wp.rcNormalPosition.top+=y-shiftscry;
  wp.rcNormalPosition.right+=x-shiftscrx;
  wp.rcNormalPosition.bottom+=y-shiftscry;
  shiftscrx=x;
  shiftscry=y;
  SetWindowPlacement(hMainWnd,&wp);
  }


#ifndef WINDOWCLASSFLAGS 
#define WINDOWCLASSFLAGS 0
#endif

static void RegisterWindowClass()
  {
  WNDCLASSEX cls;
  cls.cbSize=sizeof(cls);
  cls.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC|WINDOWCLASSFLAGS ;
  cls.lpfnWndProc=(WNDPROC)GameMainWindowWindowProc;
  cls.cbClsExtra=0;
  cls.cbWndExtra=0;
  cls.hInstance=GetModuleHandle(NULL);
  cls.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAINICON));;
  cls.hCursor=NULL;
  cls.hbrBackground=NULL;
  cls.lpszMenuName=NULL;
  cls.lpszClassName=SKELDALCLASSNAME;
  cls.hIconSm=NULL;
  if (RegisterClassEx(&cls)==NULL) 
    {
    MessageBox(NULL,"RegisterClassFailed",NULL,MB_OK);
    exit(0);
    }
  }

static void ColCalc()
  {
  do
    {
    int32_t val;
    printf("number:");
    scanf("%X",&val);
    printf("RGB555(%d,%d,%d)\n",(val>>10),(val>>5) & 0x1F, val & 0x1F);
    }
  while (1);
  }

static void CreateSkeldalWindow()
  {
  //ColCalc();
  char buff[256];

  LoadString(GetModuleHandle(NULL),IDS_WINTITLE,buff,sizeof(buff));


  RECT rc={0,0,initSizeX*dxWindowZoom/2,initSizeY*dxWindowZoom/2};
  DWORD flags=(INWINDOW?(WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION|WS_BORDER):(WS_POPUP|WS_SYSMENU))|WS_VISIBLE;
  AdjustWindowRect(&rc,flags,FALSE);
  hMainWnd=CreateWindow(SKELDALCLASSNAME,buff,flags,100,100,rc.right-rc.left,rc.bottom-rc.top,NULL,NULL,GetModuleHandle(NULL),NULL);
  if (hMainWnd==NULL)
    {
    MessageBox(NULL,"WindowCreationFailed",NULL,MB_OK);
    exit(0);
    }  
  UpdateWindow(hMainWnd);  
  }


static void DisplayMode(char init)
  {
  DEVMODE mode;
  int res;

  if (init)
	{
	EnumDisplaySettings(NULL,ENUM_REGISTRY_SETTINGS,&mode);

	mode.dmSize=sizeof(mode);
	mode.dmBitsPerPel=16;
	mode.dmFields=DM_BITSPERPEL|DM_DISPLAYFREQUENCY|DM_DISPLAYFLAGS|DM_PELSWIDTH|DM_PELSHEIGHT;

    res=ChangeDisplaySettings(&mode,0);
	}
  else
    res=ChangeDisplaySettings(NULL,0);
  } 

static bool ShowLogo()
  {
  HBITMAP logo=(HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_INITLOGO),IMAGE_BITMAP,0,0,0);
  if (logo==NULL) return false;
  HDC hDC;
  CheckResult(DxBackBuffer->GetDC(&hDC));
  HDC hBitmap=CreateCompatibleDC(hDC);
  HBITMAP old=(HBITMAP)SelectObject(hBitmap,logo);
  BitBlt(hDC,0,0,initSizeX,initSizeY,hBitmap,0,0,SRCCOPY);
  SelectObject(hBitmap,old);
  DeleteDC(hBitmap);
  DxBackBuffer->ReleaseDC(hDC);
  DeleteObject(logo);
  return true;
  }

void CheckMessageQueue();

char DXInit64(char inwindow, int zoom, int monitor, int refresh)
  {
  runinwindow=inwindow!=0;
  dxWindowZoom=inwindow?zoom+1:2;

  RegisterWindowClass();
  CreateSkeldalWindow();  

  DxHandle=Direct3DCreate9(D3D_SDK_VERSION);
  if (DxHandle==NULL)
  {
    MessageBox(hMainWnd,"Nepodarilo se inicializovat DirectX. "
      "Preinstalujte prosim DirectX ze stranek www.microsoft.com/directx. "
      "Ke spusteni je potreba mit aspon verzi DirectX 9.0c",NULL,MB_OK|MB_ICONEXCLAMATION);
    return 0;
  }
  
  HMONITOR mon=DxHandle->GetAdapterMonitor(monitor);
  if (mon!=NULL)
  {
    MONITORINFO moninfo;
    moninfo.cbSize=sizeof(moninfo);
    GetMonitorInfo(mon,&moninfo);
    SetWindowPos(hMainWnd,NULL,moninfo.rcWork.left,moninfo.rcWork.top,0,0,SWP_NOZORDER|SWP_NOSIZE);    
  }

  pparm.BackBufferWidth=initSizeX;
  pparm. BackBufferHeight=initSizeY;
  pparm.BackBufferFormat=D3DFMT_R5G6B5;
  pparm.BackBufferCount=1;
  pparm.MultiSampleType=D3DMULTISAMPLE_NONE;
  pparm.SwapEffect=D3DSWAPEFFECT_COPY;
  pparm.hDeviceWindow=hMainWnd;
  pparm.Windowed=INWINDOW;
  pparm.EnableAutoDepthStencil=FALSE;
  pparm.Flags=D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
  pparm.FullScreen_RefreshRateInHz=INWINDOW?0:refresh;
  pparm.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE ;
  pparm.MultiSampleQuality=0;

  HRESULT res;

  res=DxHandle->CreateDevice(monitor,DXDEVICE_TYPE,hMainWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pparm,&DxDevice);  
  if (res!=0 && inwindow)
  {
    DisplayMode(1);    
    res=DxHandle->CreateDevice(monitor,DXDEVICE_TYPE,hMainWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pparm,&DxDevice);  
  }
  if (res!=0)
  {
    res=DxHandle->CreateDevice(monitor,D3DDEVTYPE_REF,hMainWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pparm,&DxDevice);  
  }
  if (res!=0)
  {    
    CheckResult(res);
    return 0;
  }

  res=DxDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&DxBackBuffer);
  CheckResult(res);

  bool logo;
  logo=ShowLogo();

  D3DLOCKED_RECT lrc;

  res=DxBackBuffer->LockRect(&lrc,NULL,0);
  CheckResult(res);

  main_linelen=dx_linelen=scr_linelen=lrc.Pitch;
  scr_linelen2=scr_linelen/2;
  curBuffer=mainBuffer=(unsigned short *)lrc.pBits;

  secondBuffer=(unsigned short *)malloc(lrc.Pitch*initSizeY);

  if (logo)
	{
	InvalidateRect(hMainWnd,NULL,TRUE);
    DWORD tm=GetTickCount()+5000;
	while (tm>GetTickCount()) {Sleep(100);CheckMessageQueue();}
	}
 
  return 1;
  }

void DXCloseMode()
  {
  if (DxDevice)
	{
	if (DxBackBuffer) DxBackBuffer->Release();
	DxDevice->Release();
	DxHandle->Release();
	DestroyWindow(hMainWnd);
	UnregisterClass(SKELDALCLASSNAME,GetModuleHandle(NULL));
	free(secondBuffer);
	if (runinwindow) DisplayMode(0);
	DxDevice=NULL;
    DxBackBuffer = NULL;
	}
  }


static void HandleDeviceLost()
    {
    HRESULT res=D3DERR_DEVICELOST;
    DxBackBuffer->Release();
    DxBackBuffer=NULL;
    while (res==D3DERR_DEVICELOST)
      {
      while (DxDevice->TestCooperativeLevel()!=D3DERR_DEVICENOTRESET) 
        {
        MSG msg;
        GetMessage(&msg,0,0,0);
        DispatchMessage(&msg);
        }    
      res=DxDevice->Reset(&pparm);
      }
    res=DxDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&DxBackBuffer);    
    CheckResult(res);
    res=DxDevice->Present(NULL,NULL,hMainWnd,NULL);
    }

static void GlobalPresent(RECT *prc)
{
  RECT rc=*prc;
  RECT winrc=rc;
  int z=dxWindowZoom/2;
  if (dxWindowZoom!=2)
  {
    rc.left-=z;
    rc.top-=z;
    rc.right+=z;
    rc.bottom+=z;
    if (rc.left<0) rc.left=0;
    if (rc.top<0) rc.top=0;
    if (rc.right>=winrc.right) rc.right=winrc.right;
    if (rc.bottom>=winrc.bottom) rc.bottom=winrc.bottom;
    rc.left&=~1;
    rc.top&=~1;
    rc.right&=~1;
    rc.bottom&=~1;
    winrc=rc;  
	winrc.left=winrc.left*dxWindowZoom/2;
	winrc.top=winrc.top*dxWindowZoom/2;
	winrc.right=winrc.right*dxWindowZoom/2;
	winrc.bottom=winrc.bottom*dxWindowZoom/2;  
  }
  if (!dialogs)
	{
	LRESULT res=DxDevice->Present(&rc,&winrc,hMainWnd,NULL);
	if (res==D3DERR_DEVICELOST) HandleDeviceLost();
	}
}


static bool UnLockBuffers()
  {
  if (DxBackBuffer==NULL) return false;
  HRESULT res;
  res=DxBackBuffer->UnlockRect();
  CheckResult(res);
  return true;
  }

static void LockBuffers()
  {
  D3DLOCKED_RECT lrc;
  HRESULT res;
  res=DxBackBuffer->LockRect(&lrc,NULL,0);
  CheckResult(res);

  curBuffer=mainBuffer=(unsigned short *)lrc.pBits;
  if (dx_linelen!=lrc.Pitch)
    {
    free(secondBuffer);
    main_linelen=dx_linelen=scr_linelen=lrc.Pitch;
    scr_linelen2=scr_linelen/2;
    curBuffer=mainBuffer=(unsigned short *)lrc.pBits;
    secondBuffer=(unsigned short *)malloc(lrc.Pitch*initSizeY);
    }
  }

void DXCopyRects64(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys)
  {
  if (UnLockBuffers()==false) return;

  RECT rc;
  rc.left=x;
  rc.top=y;
  rc.right=x+xs;
  rc.bottom=y+ys;
  GlobalPresent(&rc);
  LockBuffers();

  }

unsigned short *GetScreenAdr()
  {
  return curBuffer;
  }

unsigned short *GetBuffer2nd()
  {
  return secondBuffer;
  }

void RedirectScreen(unsigned short *newaddr)
  {
  curBuffer=newaddr;
  }

void RestoreScreen()
  {
  curBuffer=mainBuffer;
  scr_linelen=main_linelen;
  scr_linelen2=main_linelen>>1;

  }


void RedirectScreenBufferSecond()
  {
  curBuffer=secondBuffer;
  }


void DXCopyRects64zoom2(unsigned short x,unsigned short y,unsigned short xs,unsigned short ys)
  {
  HRESULT res;
  if (UnLockBuffers()==false) return;

  
  RECT rc;
  rc.left=x;
  rc.top=y;
  rc.right=x+xs;
  rc.bottom=y+ys;
  RECT zoom;
  zoom.left=x;
  zoom.top=y;
  zoom.right=x+2*xs;
  zoom.bottom=y+2*ys;

  res=DxDevice->Present(&rc,&zoom,hMainWnd,NULL);
  if (res==D3DERR_DEVICELOST) HandleDeviceLost();
  

  LockBuffers();  
  }


void *DxPrepareWalk(int ypos)
  {
  HRESULT res;


  IDirect3DSurface9 *tempbuff;
  res=DxDevice->CreateRenderTarget(640,360,D3DFMT_R5G6B5,D3DMULTISAMPLE_NONE,0,TRUE,&tempbuff,NULL);
  CheckResult(res);
  UnLockBuffers();
  RECT src;src.left=0;src.top=ypos;src.right=640;src.bottom=ypos+360;
  RECT trg;trg.left=0;trg.top=0;trg.right=640;trg.bottom=360;
  res=DxDevice->StretchRect(DxBackBuffer,&src,tempbuff,&trg,D3DTEXF_NONE);
  CheckResult(res);
  LockBuffers();

  return (void *)tempbuff;
  }

void DxZoomWalk(void *handle, int ypos, int *points,float phase, void *lodka)
  {
  if (phase>1.0) phase=1.0f;
  if (phase<0.0) phase=0.0f;
  UnLockBuffers();
  IDirect3DSurface9 *surf=(IDirect3DSurface9 *)handle;
  RECT rc1;rc1.left=0;rc1.top=0;rc1.right=640;rc1.bottom=360;
  RECT rc2;rc2.left=points[0];rc2.top=points[1];rc2.right=points[2];rc2.bottom=points[3];
  RECT rcx;
  rcx.left=(int)(rc1.left+(rc2.left-rc1.left)*phase);
  rcx.top=(int)(rc1.top+(rc2.top-rc1.top)*phase);
  rcx.right=(int)(rc1.right+(rc2.right-rc1.right)*phase);
  rcx.bottom=(int)(rc1.bottom+(rc2.bottom-rc1.bottom)*phase);
  rc1.top+=ypos;
  rc1.bottom+=ypos;

/*  HDC srcdc;
  HDC trgdc;
  HRESULT res;
  res=surf->GetDC(&srcdc);CheckResult(res);
  res=DxBackBuffer->GetDC(&trgdc);CheckResult(res);
  StretchBlt(
	     trgdc,rc1.left,rc1.top,rc1.right-rc1.left,rc1.bottom-rc1.top,
		 srcdc,rcx.left,rcx.top,rcx.right-rcx.left,rcx.bottom-rcx.top,SRCCOPY);
  surf->ReleaseDC(srcdc);
  DxBackBuffer->ReleaseDC(trgdc);
*/


  HRESULT res;
  res=DxDevice->StretchRect(surf,&rcx,DxBackBuffer,&rc1,D3DTEXF_NONE);
  CheckResult(res);

  GlobalPresent(&rc1);
  LockBuffers();
  }

void DxDoneWalk(void *handle)
  {
  IDirect3DSurface9 *surf=(IDirect3DSurface9 *)handle;
  if (surf) surf->Release();
  }



void DxTurnLeftRight(char right, float phase, int border, int ypos, int *last)
  {
  if (phase>1.0) phase=1.0f;
  if (phase<0.0) phase=0.0f;
  if (phase<0.05f) return;
  int space=640-2*border;
  
  unsigned short *scr=GetScreenAdr()+scr_linelen2*ypos;
  unsigned short *buf=GetBuffer2nd()+scr_linelen2*ypos;  

  int turnout=(float)(space*phase)+border;
  int difto=turnout-*last;
  if (!right)
    {
    buf+=border;
    for (int y=0;y<360;y++)
      {
      memcpy(scr,scr+difto,(640-turnout)*2);
      memcpy(scr+(640-turnout),buf,(turnout)*2);
      scr+=scr_linelen2;
      buf+=scr_linelen2;
      }      
    *last=turnout;
    }
  else
    {
    for (int y=0;y<360;y++)
      {
      memmove(scr+*last+difto-border,scr+*last-border,(640-turnout+border)*2);
      memcpy(scr,buf+(640-turnout)-border,turnout*2);
      scr+=scr_linelen2;
      buf+=scr_linelen2;
      }
    *last=turnout;
    }
  DXCopyRects64(0,ypos,640,360+ypos);
  }

void *DxPrepareTurn(int ypos)
  {
  IDirect3DSurface9 **handle=new IDirect3DSurface9 *[2];
  HRESULT res;


  res=DxDevice->CreateRenderTarget(640,360,D3DFMT_R5G6B5,D3DMULTISAMPLE_NONE,0,TRUE,handle,NULL);
  CheckResult(res);
  UnLockBuffers();
  RECT src;src.left=0;src.top=ypos;src.right=640;src.bottom=ypos+360;
  RECT trg;trg.left=0;trg.top=0;trg.right=640;trg.bottom=360;
  res=DxDevice->StretchRect(DxBackBuffer,&src,handle[0],&trg,D3DTEXF_NONE);
  CheckResult(res);

  res=DxDevice->CreateRenderTarget(640,360,D3DFMT_R5G6B5,D3DMULTISAMPLE_NONE,0,TRUE,handle+1,NULL);
  CheckResult(res);
  D3DLOCKED_RECT lrect;
  res=handle[1]->LockRect(&lrect,NULL,0);
  CheckResult(res);
  
  unsigned short *trgptr=(unsigned short *)lrect.pBits;
  unsigned short *srcptr=secondBuffer+scr_linelen2*ypos;
  for (int i=0;i<360;i++,trgptr=(unsigned short *)((char *)trgptr+lrect.Pitch),srcptr+=scr_linelen2)
	memcpy(trgptr,srcptr,640*2);
  handle[1]->UnlockRect();

  LockBuffers();
  
  return (void *)handle;
  }


static inline void CopySurfaceAtPos(IDirect3DSurface9 *src, int width, int height, int xpos,int ypos, int border)
  {
  RECT trc;
  trc.left=xpos;
  trc.top=ypos;
  trc.right=xpos+width;
  trc.bottom=ypos+height;
  RECT s_rc;
  s_rc.left=0;
  s_rc.top=0;
  s_rc.right=width;
  s_rc.bottom=height;
  if (border<0) 
	{
	trc.left-=border;
	s_rc.left-=border;
	}
  else
	{
	trc.right-=border;
	s_rc.right-=border;
	}
  if (trc.left<0) {s_rc.left-=trc.left*0.7;trc.left=0;}
  if (trc.right>=640) {s_rc.right-=(trc.right-640)*0.7;trc.right=640;}
  CheckResult(DxDevice->StretchRect(src,&s_rc,DxBackBuffer,&trc,D3DTEXF_NONE));
  }

void DxTurn(void *handle, char right, int ypos,int border, float phase, void *lodka)
  {
  right=!right;
 if (phase>1.0) phase=1.0f;
 if (phase<0.0) phase=0.0f;
 UnLockBuffers();
  IDirect3DSurface9 **ihandle=(IDirect3DSurface9 **)handle;
  IDirect3DSurface9 *sleft=ihandle[right?0:1];
  IDirect3DSurface9 *sright=ihandle[right?1:0];
  int width=640-border*2;
  int xpos=(int)border+width*phase;
  if (right) xpos=640-xpos;
  CopySurfaceAtPos(sleft,640,360,xpos-640+border,ypos,border);
  CopySurfaceAtPos(sright,640,360,xpos-border,ypos,-border);


  RECT rc={0,ypos,640,ypos+360};

  GlobalPresent(&rc);
  LockBuffers();
  }


void DxDoneTurn(void *handle)
  {
  IDirect3DSurface9 **ihandle=(IDirect3DSurface9 **)handle;
  ihandle[0]->Release();
  ihandle[1]->Release();
  delete [] ihandle;  
  }

void DxDialogs(char enable)
  {
  dialogs=enable!=0;
  }
  
}
HWND GetGameWindow() 
  {return hMainWnd;}

void DxLockBuffers(BOOL lock)
{
  if (lock) LockBuffers();
  else UnLockBuffers();
}

void StripBlt(void *data, unsigned int startline, uint32_t width)
{
  unsigned short *start=startline*scr_linelen2+GetScreenAdr();
  while (width--)
  {
	memcpy(start,data,640*2);
	data=(void *)((char *)data+scr_linelen);
	start=start+scr_linelen2;
  }
}

void DxSetInitResolution(int x, int y)
{
    initSizeX = x;
    initSizeY = y;

}

int DxGetResX() {return initSizeX;}
int DxGetResY() {return initSizeY;}
