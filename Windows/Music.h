#ifndef __SKELDAL_MUSIC_
#define __SKELDAL_MUSIC_

#include "WAPlayer.h"

#define DWORD_PTR DWORD *
#include <dsound.h>


class MusicPlayer: public IWAOutput
{
  DWORD _speed;   //speed relative to sampling frequence;
  bool _stereo;   //true, if stereo
  bool _bit16;    //true, if 16 bits
  bool _opened;
  IDirectSoundBuffer8 *_ds8Buffer;
  DWORD _lastWritePos;  //last write pos;
  DWORD _minorpos;
  int *_linvoltable;
  DWORD _crossfadebytes;
  unsigned char _volume;
  CRITICAL_SECTION _lock;
  bool _paused;

  DWORD GetSafeXFadePos();
  
public:
  MusicPlayer();
  ~MusicPlayer();

  HRESULT InitBuffer(IDirectSound8 *ds8, int *linvoltable);
  HRESULT Play();
  HRESULT Done();
  int Open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms);
  void Close();
  int Write(const char *buf, int len);
  int CanWrite();
  int IsPlaying();
  void SetVolume(int volume);
  void SetPan(int pan) {}
  void Flush(int t) {}
  int GetOutputTime() {return 1;}
  int GetWrittenTime() {return 1;}
  unsigned long AddRef() {return 1;}
  unsigned long Release() {return 1;}
  int Pause(int pause);
  bool IsOpenned() {return _opened;}
};

#pragma pack (1)
struct MusFileHeader
    {
    short channels;
    long  freq;
    long  ssize;
    long  blocks;
    long  reserved1;
    long  reserved2;
    short ampltable[256];
    };
  #pragma pack()


class IDecoder
{
public:
  virtual void AttachOutput(IWAOutput *o)=0;
  virtual bool Play(const char *filename)=0;
  virtual bool IsPlaying()=0;
  virtual void Stop()=0;
  virtual void SetVolume(int volume, int main)=0;
  virtual bool NotUsingOutput()=0;
  virtual ~IDecoder() {}
};

class MusDecoder: public IDecoder
{
  MusFileHeader _header;
  bool _playing;
  HANDLE _thread;  
  IWAOutput *_output;
  HANDLE _file;
  bool _stop;


  static DWORD WINAPI StartMusDecoder(LPVOID data);
  static DWORD WINAPI StartSilentWritter(LPVOID data);
  UINT MusDecodingThread();
  UINT SilentWritterThread();
public:
  MusDecoder();
  ~MusDecoder();

  void AttachOutput(IWAOutput *o);

  bool Play(const char *filename);
  void Stop();
  bool IsPlaying() {return _playing;}
  void SetVolume(int volume, int main) {if (_output) _output->SetVolume(volume);}
  bool NotUsingOutput() {return false;}
};

class WinAmpDecoder: public IDecoder, public WAPlayer
{  
  IWAOutput *_output;
  WAInputPlugin *_currPlugin;
  bool _nooutput;
public:
  WinAmpDecoder();
  ~WinAmpDecoder();
  
  void AttachOutput(IWAOutput *o);
  bool Play(const char *filename);
  void Stop();
  bool IsPlaying();
  void SetVolume(int volume, int main);
  bool NotUsingOutput() {return _nooutput;}
};


#endif