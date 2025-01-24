// WAInputPlugin.cpp: implementation of the WAInputPlugin class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WAInputPlugin.h"
#include <malloc.h>
#include <direct.h>
#include "wa_ipc.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_PLUGINS_AT_TIME 16

#define DECLARE_TEMPLATES(x) \
  x(0)\
  x(1)\
  x(2)\
  x(3)\
  x(4)\
  x(5)\
  x(6)\
  x(7)\
  x(8)\
  x(9)\
  x(10)\
  x(11)\
  x(12)\
  x(13)\
  x(14)\
  x(15)\
/*  x(16)\
  x(17)\
  x(18)\
  x(19)\
  x(20)\
  x(21)\
  x(22)\
  x(23)\
  x(24)\
  x(25)\
  x(26)\
  x(27)\
  x(28)\
  x(29)\
  x(30)\
  x(31)\*/
  

#define JOIN(x,y) &x##y

#define DECLARE_TEMPLATE_ARRAYS(x) \
static void *p_##x[]={JOIN(x,_0),JOIN(x,_1),JOIN(x,_2),JOIN(x,_3),JOIN(x,_4),JOIN(x,_5),JOIN(x,_6),JOIN(x,_7),\
JOIN(x,_8),JOIN(x,_9),JOIN(x,_10),JOIN(x,_11),JOIN(x,_12),JOIN(x,_13),JOIN(x,_14),JOIN(x,_15)/*,\
JOIN(x,_16),JOIN(x,_17),JOIN(x,_18),JOIN(x,_19),JOIN(x,_20),JOIN(x,_21),JOIN(x,_22),JOIN(x,_23),\
JOIN(x,_24),JOIN(x,_25),JOIN(x,_26),JOIN(x,_27),JOIN(x,_28),JOIN(x,_29),JOIN(x,_30),JOIN(x,_31)*/\
};


WAInputPlugin::WAInputPlugin()
{
_inPlugin=0;
_mod=0;
_ioutput=0;
_finished=false;
_hNotify=0;
}

WAInputPlugin::~WAInputPlugin()
{
UnloadPlugin();
}

typedef In_Module * (*type_winampGetInModule2)();

static void empty_SAVSAInit(int maxlatency_in_ms, int srate)
{

}

static void empty_SAVSADeInit()
{

}

static void empty_SAAddPCMData(void *PCMData, int nch, int bps, int timestamp)
{

}

static int empty_SAGetMode()
{
return 1;
}

static void empty_SAAdd(void *data, int timestamp, int csa)
{

}

static void empty_VSAAddPCMData(void *PCMData, int nch, int bps, int timestamp)
{

}

static void empty_VSAAdd(void *data, int timestamp)
{

}

static int empty_VSAGetMode(int *specNch, int *waveNch)
{
  return 0;
}

static void empty_VSASetInfo(int nch, int srate)
{

}

static int empty_dsp_isactive()
{
  return 0;
}

static int empty_dsp_dosamples(short int *samples, int numsamples, int bps, int nch, int srate)
{
  return numsamples;
}

static void empty_SetInfo(int bitrate, int srate, int stereo, int synched)
{

}

static void empty_About(HWND hWnd)
{

}

static void empty_Config(HWND hWnd)
{

}

static void empty_Init()
{

}
static void empty_Quit()
{

}


class RegistredPlugsArray
{
  WAInputPlugin *_arr[MAX_PLUGINS_AT_TIME];
public:
  RegistredPlugsArray() {memset(_arr,0,sizeof(_arr));}
  int Register(WAInputPlugin *plug);
  void Unregister(WAInputPlugin *plug);
  WAInputPlugin *operator[] (int index) {return _arr[index];}
};

int RegistredPlugsArray::Register(WAInputPlugin *plug)
{
  for (int i=0;i<MAX_PLUGINS_AT_TIME;i++) if (_arr[i]==0)
  {
	_arr[i]=plug;
	return i;
  }
  return -1;
}

void RegistredPlugsArray::Unregister(WAInputPlugin *plug)
{
  for (int i=0;i<MAX_PLUGINS_AT_TIME;i++) if (_arr[i]==plug) _arr[i]=0;  
}

static RegistredPlugsArray GRegistredPlugsArray;

#define m_Open(index)\
static int output_Open_##index(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->Open(samplerate, numchannels, bitspersamp, bufferlenms, prebufferms);\
  return -1;\
}

#define m_Close(index)\
static void output_Close_##index()\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) o->Close();\
}

#define m_Write(index)\
static int output_Write_##index(char *buff, int len)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->Write(buff,len);\
  return 1;\
}

#define m_CanWrite(index)\
static int output_CanWrite_##index()\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->CanWrite();\
  return 0;\
}

#define m_IsPlaying(index)\
static int output_IsPlaying_##index()\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  int p=o->IsPlaying();\
  return p;\
}

#define m_Pause(index)\
static int output_Pause_##index(int pause)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->Pause(pause);\
  return 0;\
}

#define m_SetVolume(index)\
static void output_SetVolume_##index(int volume)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) o->SetVolume(volume);\
}

#define m_SetPan(index)\
static void output_SetPan_##index(int pan)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) o->SetPan(pan);\
}

#define m_Flush(index)\
static void output_Flush_##index(int t)\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) o->Flush(t);\
}

#define m_GetOutputTime(index)\
static int output_GetOutputTime_##index()\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->GetOutputTime();\
  return 0;\
}

#define m_GetWrittenTime(index)\
static int output_GetWrittenTime_##index()\
{\
  IWAOutput *o=GRegistredPlugsArray[index]->GetAttachedOutputNoAddRef();\
  if (o) return o->GetWrittenTime();\
  return 0;\
}

DECLARE_TEMPLATES(m_Open);
DECLARE_TEMPLATES(m_Close);
DECLARE_TEMPLATES(m_Write);
DECLARE_TEMPLATES(m_CanWrite);
DECLARE_TEMPLATES(m_IsPlaying);
DECLARE_TEMPLATES(m_Pause);
DECLARE_TEMPLATES(m_SetVolume);
DECLARE_TEMPLATES(m_SetPan);
DECLARE_TEMPLATES(m_Flush);
DECLARE_TEMPLATES(m_GetOutputTime);
DECLARE_TEMPLATES(m_GetWrittenTime);

DECLARE_TEMPLATE_ARRAYS(output_Open);
DECLARE_TEMPLATE_ARRAYS(output_Close);
DECLARE_TEMPLATE_ARRAYS(output_Write);
DECLARE_TEMPLATE_ARRAYS(output_CanWrite);
DECLARE_TEMPLATE_ARRAYS(output_IsPlaying);
DECLARE_TEMPLATE_ARRAYS(output_Pause);
DECLARE_TEMPLATE_ARRAYS(output_SetVolume);
DECLARE_TEMPLATE_ARRAYS(output_SetPan);
DECLARE_TEMPLATE_ARRAYS(output_Flush);
DECLARE_TEMPLATE_ARRAYS(output_GetOutputTime);
DECLARE_TEMPLATE_ARRAYS(output_GetWrittenTime);


WAInputPlugin::PluginError WAInputPlugin::UIConfig(HWND parentHwnd)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->Config(parentHwnd);
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::UIAbout(HWND parentHwnd)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->About(parentHwnd);
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::UnloadPlugin()
{
  if (_inPlugin!=0) 
  {
    __try
    {
	_inPlugin->Quit();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      _mod=0; //prevent calling FreeLibrary.... plugin cannot be unloaded....
    }
	_inPlugin=0;
  }
  if (_hNotify)
  {
    DestroyWindow(_hNotify);
    _hNotify=0;
  }
  if (_mod)
  {
	FreeLibrary(_mod);
	_mod=0;
  }
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::IsLoaded()
{
  if (_inPlugin==0) return errNotLoaded;
  return errOk;
}

const char *WAInputPlugin::GetDescription()
{
  if (_inPlugin==0) return 0;
  return _inPlugin->description;
}

WAInputPlugin::PluginError WAInputPlugin::GetFileInfo(const char *filename, char *title, int title_size, int *len_in_ms)
{
  if (_inPlugin==0) return errNotLoaded;  
  char *buff=new char[4096];
  DWORD  r=GetTickCount();
  memcpy(buff+4092,&r,4);
  _inPlugin->GetFileInfo(const_cast<char *>(filename),buff,len_in_ms);
  if (memcmp(buff+4092,&r,4)==0)
  {
	return errPluginBufferOverrun;
  }
  int len=strlen(buff);
  if (len>=title_size)
  {
	strncpy(title,buff,title_size);
	return errBufferTooSmall;
  }
  else
  {
	strcpy(title,buff);
	return errOk;
  }
}

WAInputPlugin::PluginError WAInputPlugin::UIFileInfo(const char *filename, HWND parentHwnd)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->InfoBox(const_cast<char *>(filename),parentHwnd);
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::CanPlayFile(const char *filename)
{
  if (_inPlugin==0) return errNotLoaded;
  if (_inPlugin->IsOurFile(const_cast<char *>(filename))) return errOk;
  char *fext=_inPlugin->FileExtensions;
  const char *bslash=strrchr(filename,'\\');
  if (bslash==0) bslash=filename;
  const char *ext=strrchr(bslash,'.');
  if (ext==0) ext=strchr(bslash,0);
  else ext++;
  int extlen=strlen(ext);
  while (*fext)
  {
	if (strnicmp(fext,ext,extlen)==0 && (fext[extlen]==';' || fext[extlen]==0))
	  return errOk;
	char *c=strchr(fext,';');
	if (c==0) 
	{
	  c=strchr(fext,0);
	  c++;
	  if (*c) c=strchr(c,0)+1;
	  fext=c;
	}
	else
	  fext=c+1;
  }
  return errUnknownFileFormat;
} 

WAInputPlugin::PluginError WAInputPlugin::AttachOutput(IWAOutput *o)
{
  if (_ioutput==o) return errOk;  
  if ((_ioutput!=0)!=(o!=0))
  {
	if (o==0) GRegistredPlugsArray.Unregister(this);
	else 
	{
	  int plugid=GRegistredPlugsArray.Register(this);
	  if (plugid==-1) return errTooManyPlugins;
  
	  _output.About=empty_About;
	  _output.Config=empty_Config;
	  _output.Init=empty_Init;
	  _output.Quit=empty_Quit;
	  *(void **)&_output.CanWrite=p_output_CanWrite[plugid];
	  *(void **)&_output.Close=p_output_Close[plugid];
	  *(void **)&_output.Flush=p_output_Flush[plugid];
	  *(void **)&_output.GetOutputTime=p_output_GetOutputTime[plugid];
	  *(void **)&_output.GetWrittenTime=p_output_GetWrittenTime[plugid];
	  *(void **)&_output.IsPlaying=p_output_IsPlaying[plugid];
	  *(void **)&_output.Open=p_output_Open[plugid];
	  *(void **)&_output.Pause=p_output_Pause[plugid];
	  *(void **)&_output.SetPan=p_output_SetPan[plugid];
	  *(void **)&_output.SetVolume=p_output_SetVolume[plugid];
	  *(void **)&_output.Write=p_output_Write[plugid];
	  _output.hDllInstance=GetModuleHandle(0);
	  _output.hMainWindow=0;
	  _output.description="Emulated output device";
	  _output.version=0x100;
	  _output.id=999999999;
	}
  }
  if (_ioutput!=0) _ioutput->Release();
  _ioutput=o;
  if (o) o->AddRef();
  return errOk;
}

#define WM_WA_MPEG_EOF WM_USER+2
#define IPC_GET_API_SERVICE 3025


static LRESULT NotifyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_WA_MPEG_EOF:
	{
	  WAInputPlugin *o=(WAInputPlugin *)GetWindowLong(hWnd,GWL_USERDATA);
	  if (o) o->MarkFinished();
	  return 0;
	}
  case WM_USER:
    {
      switch (lParam)
      {
      case IPC_GETINIFILE:
        {
          return (LRESULT)("skeldalmusic.ini");
        }
      case IPC_GETINIDIRECTORY:
        {
          return (LRESULT)(".\\");
        }
      case IPC_GET_API_SERVICE: return 0;
      case IPC_GET_RANDFUNC: return (LRESULT)&rand;
      }
    }
  default: return DefWindowProc(hWnd,msg,wParam,lParam);
  }
}

static const char *RegisterWAPluginWnd()
{
  const char *name="WAPluginWnd";
  WNDCLASS cls;
  if (GetClassInfo(GetModuleHandle(0),name,&cls)==FALSE)
  {
	memset(&cls,0,sizeof(cls));
	cls.lpszClassName=name;
	cls.hInstance=GetModuleHandle(0);
	cls.lpfnWndProc=(WNDPROC)NotifyWndProc;
	RegisterClass(&cls);
  }
  return name;
}

WAInputPlugin::PluginError WAInputPlugin::LoadPlugin(const char *name)
{  
  UnloadPlugin();
  char *cwd=_getcwd(0,0);
  char *setcwd=strcpy((char *)alloca(strlen(name)+1),name);
  char *bs=const_cast<char *>(strrchr(setcwd,'\\'));
  if (bs)
  {
    *bs=0;
    bs=const_cast<char *>(strrchr(setcwd,'\\'));
    if (bs) *bs=0;
  }
  _chdir(setcwd);
  SetErrorMode(SEM_FAILCRITICALERRORS);
  _mod=LoadLibrary(name);
  _chdir(cwd);
  free(cwd);
  if (_mod==0) return errNotFound;
  type_winampGetInModule2 winampGetInModule2=(type_winampGetInModule2)GetProcAddress(_mod,"winampGetInModule2");
  if (winampGetInModule2==0) return errNotValidPlugin;
  _inPlugin=winampGetInModule2();
  if (_inPlugin==0) return errCantInit;
  _hNotify=CreateWindow(RegisterWAPluginWnd(),"",0,0,0,0,0,0,0,GetModuleHandle(0),0);
  SetWindowLong(_hNotify,GWL_USERDATA,(LONG)this);
  _inPlugin->hMainWindow=_hNotify;
  _inPlugin->hDllInstance=_mod;
  _inPlugin->SAVSAInit=empty_SAVSAInit;
  _inPlugin->SAVSADeInit=empty_SAVSADeInit;
  _inPlugin->SAAddPCMData=empty_SAAddPCMData;
  _inPlugin->SAGetMode=empty_SAGetMode;
  _inPlugin->SAAdd=empty_SAAdd;
  _inPlugin->VSAAddPCMData=empty_VSAAddPCMData;
  _inPlugin->VSAGetMode=empty_VSAGetMode;
  _inPlugin->VSAAdd=empty_VSAAdd;
  _inPlugin->VSASetInfo=empty_VSASetInfo;
  _inPlugin->dsp_isactive=empty_dsp_isactive;
  _inPlugin->dsp_dosamples=empty_dsp_dosamples;
  _inPlugin->SetInfo=empty_SetInfo;
  _inPlugin->outMod=0;
  __try
  {
    _inPlugin->Init();
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    __try
    {
      _inPlugin->Quit();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      _mod=0; //Free library cannot be called, because plugin cannot be terminated... memory and resource leak
    }
    _inPlugin=0;
    return errGPF;
  }
  _inPlugin->outMod=&_output;
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::Play(const char *name)
{
  if (_inPlugin==0) return errNotLoaded;
  _finished=false;
  int i=_inPlugin->Play(const_cast<char *>(name));
  if (i==0) return errOk;
  if (i==-1) return errFileNotFound;
  return errCantPlay; 
}

WAInputPlugin::PluginError WAInputPlugin::Pause()
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->Pause();
  return errOk;
}
WAInputPlugin::PluginError WAInputPlugin::UnPause()
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->UnPause();
  return errOk;

}

WAInputPlugin::PluginError WAInputPlugin::IsPaused()
{
   if (_inPlugin==0) return errNotLoaded;
   if (_inPlugin->IsPaused()) return errOk;
   return errFalse;
}

WAInputPlugin::PluginError WAInputPlugin::Stop()
{
  if (_inPlugin==0) return errNotLoaded;  
  _inPlugin->Pause();
  _inPlugin->UnPause();
  _inPlugin->Stop();
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::SetOutputTime(int t)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->SetOutputTime(t);
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::SetVolume(int volume)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->SetVolume(volume);
  return errOk;

}
WAInputPlugin::PluginError WAInputPlugin::SetPan(int pan)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->SetPan(pan);
  return errOk;
}

WAInputPlugin::PluginError WAInputPlugin::EQSet(int on, char data[10], int preamp)
{
  if (_inPlugin==0) return errNotLoaded;
  _inPlugin->EQSet(on,data,preamp);
  return errOk;
}

bool WAInputPlugin::IsFinished()
{
  MSG msg;
  if (PeekMessage(&msg,_hNotify,WM_WA_MPEG_EOF,WM_WA_MPEG_EOF,PM_REMOVE))
  {
	DispatchMessage(&msg);
  }
  return _finished;
}