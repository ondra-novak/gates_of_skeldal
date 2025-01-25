// WAInputPlugin.h: interface for the WAInputPlugin class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAINPUTPLUGIN_H__C78D396A_67F6_473B_891C_D6D162312554__INCLUDED_)
#define AFX_WAINPUTPLUGIN_H__C78D396A_67F6_473B_891C_D6D162312554__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "winamp/in2.h"

class IWAOutput
{
public:
  
	  // returns >=0 on success, <0 on failure
	  // NOTENOTENOTE: bufferlenms and prebufferms are ignored in most if not all output plug-ins. 
	  //    ... so don't expect the max latency returned to be what you asked for.
	  // returns max latency in ms (0 for diskwriters, etc)
	  // bufferlenms and prebufferms must be in ms. 0 to use defaults. 
	  // prebufferms must be <= bufferlenms
  virtual int Open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)=0;

      // close the ol' output device.
  virtual void Close()=0; 

      // 0 on success. Len == bytes to write (<= 8192 always). buf is straight audio data. 
	  // 1 returns not able to write (yet). Non-blocking, always.
  virtual int Write(const char *buf, int len)=0;
  
	  // returns number of bytes possible to write at a given time. 
	  // Never will decrease unless you call Write (or Close, heh)
  virtual int CanWrite()=0;

     // non0 if output is still going or if data in buffers waiting to be
	 // written (i.e. closing while IsPlaying() returns 1 would truncate the song
  virtual int IsPlaying()=0;
	
     // returns previous pause state
  virtual int Pause(int pause)=0;

	// volume is 0-255
  virtual void SetVolume(int volume)=0;
   // pan is -128 to 128
  virtual void SetPan(int pan)=0;

    // flushes buffers and restarts output at time t (in ms) 
	// (used for seeking)
  virtual void Flush(int t)=0;	
    
    // returns played time in MS
  virtual int GetOutputTime()=0;

    // returns time written in MS (used for synching up vis stuff)
  virtual int GetWrittenTime()=0;

  virtual uint32_t AddRef()=0;

  virtual uint32_t Release()=0;

};


class WAInputPlugin  
{ 
	HMODULE _mod;
	In_Module *_inPlugin;
	Out_Module _output;
	IWAOutput *_ioutput;
	bool _finished;
	HWND _hNotify;
    WAInputPlugin &operator=(const WAInputPlugin &other);
    WAInputPlugin(const WAInputPlugin &other);
public:
	enum PluginError
	{
	  errOk=0,
	  errNotFound,
	  errCantInit,	  
	  errNotValidPlugin,
	  errNotLoaded,
	  errBufferTooSmall,
	  errPluginBufferOverrun,
	  errUnknownFileFormat,
	  errTooManyPlugins,
	  errFileNotFound,
	  errCantPlay,
	  errFalse,
      errGPF
	};
public:
	WAInputPlugin();
	virtual ~WAInputPlugin();

	PluginError LoadPlugin(const char *name);
	PluginError UIConfig(HWND parentHwnd=0);
	PluginError UIAbout(HWND parentHwnd=0);
	PluginError UnloadPlugin();
	PluginError IsLoaded();
	const char *GetDescription();
	PluginError GetFileInfo(const char *filename, char *title, int title_size, int *len_in_ms);
	PluginError UIFileInfo(const char *filename, HWND parentHwnd=0);
	PluginError CanPlayFile(const char *filename);
	PluginError AttachOutput(IWAOutput *o);
	IWAOutput *GetAttachedOutput() {if (_ioutput) _ioutput->AddRef();return _ioutput;}
	IWAOutput *GetAttachedOutputNoAddRef() {return _ioutput;}
	PluginError Play(const char *name);
	PluginError Pause();
	PluginError UnPause();
	PluginError IsPaused();
	PluginError Stop();
	int GetLength() 
	  {if (_inPlugin) return _inPlugin->GetLength();return -1;}
	int GetOutputTime()
	  {if (_inPlugin) return _inPlugin->GetOutputTime();return -1;}
	PluginError SetOutputTime(int t);		
	PluginError SetVolume(int volume);	// from 0 to 255.. usually just call outMod->SetVolume
	PluginError SetPan(int pan);	// from -127 to 127.. usually just call outMod->SetPan
	
	PluginError EQSet(int on, char data[10], int preamp); // 0-64 each, 31 is +0, 0 is +12, 63 is -12. Do nothing to ignore.
	bool IsFinished();
    bool IsUsesOutput() {return _inPlugin->UsesOutputPlug!=0;}

	void MarkFinished() {_finished=true;}
	
};

#endif // !defined(AFX_WAINPUTPLUGIN_H__C78D396A_67F6_473B_891C_D6D162312554__INCLUDED_)
