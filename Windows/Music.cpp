#include "skeldal_win.h"
#include <malloc.h>
#include <debug.h>
#include <stdio.h>
#define DWORD_PTR DWORD *
#include <dsound.h>
#include "types.h"
#include "zvuk.h"
#include <math.h>
#include <assert.h>
extern "C" {
#include <memman.h>
  }
#include "Music.h"


#define BUFFER_SIZE (512*1024)
#define LOCK_GRANUALITY 16384

MusicPlayer::MusicPlayer()
{
  _ds8Buffer=0;
  _volume=255;
  InitializeCriticalSection(&_lock);
}

MusicPlayer::~MusicPlayer()
{
  Done();
  DeleteCriticalSection(&_lock);
}

HRESULT MusicPlayer::InitBuffer(IDirectSound8 *ds8, int *linvoltable)
{
  HRESULT hres;

  WAVEFORMATEX wfex;
  wfex.cbSize=sizeof(wfex);
  wfex.wBitsPerSample=16;
  wfex.nBlockAlign=4;
  wfex.nSamplesPerSec=44100;
  wfex.nAvgBytesPerSec=wfex.nSamplesPerSec*wfex.nBlockAlign;
  wfex.wFormatTag=WAVE_FORMAT_PCM;
  wfex.nChannels=2;
    
  DSBUFFERDESC desc;
  desc.dwSize=sizeof(desc);
  desc.dwBufferBytes=BUFFER_SIZE;
  desc.dwReserved=0;
  desc.dwFlags=DSBCAPS_CTRLVOLUME ;
  desc.lpwfxFormat=&wfex;

  IDirectSoundBuffer *dsbf;

  hres=ds8->CreateSoundBuffer(&desc,&dsbf,NULL);
  hres=dsbf->QueryInterface(IID_IDirectSoundBuffer8,(void **)&_ds8Buffer);
  dsbf->Release();

  _linvoltable=linvoltable;
  return hres;
}

HRESULT MusicPlayer::Play()
{
  _lastWritePos=0;
  DWORD size;
  void *ptr;

  _ds8Buffer->Lock(0,0,&ptr,&size,NULL,NULL,DSBLOCK_ENTIREBUFFER);
  memset(ptr,0,size);
  _ds8Buffer->Unlock(ptr,size,NULL,NULL);
  _ds8Buffer->SetVolume(_linvoltable[_volume]);
  HRESULT res=_ds8Buffer->Play(0,0,DSBPLAY_LOOPING);    
  _crossfadebytes=0;
  _minorpos=0;
  return res;
}

class AutoCloseCriticalSection
{
  LPCRITICAL_SECTION lcrit;
public:
  AutoCloseCriticalSection(LPCRITICAL_SECTION l):lcrit(l) 
  {
    EnterCriticalSection(lcrit);
  }
  ~AutoCloseCriticalSection()
  {
    LeaveCriticalSection(lcrit);
  }
};

int MusicPlayer::Open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)
{  
  AutoCloseCriticalSection lsect(&_lock);  
  if (numchannels<1 || numchannels>2) return -1;
  if (bitspersamp!=8 && bitspersamp!=16) return -1;
  _stereo=numchannels==2;
  _bit16=bitspersamp==16;
  _speed=samplerate*1024/44100;
  if (_speed<128) return -1;
  _opened=true;  
  return 0;
}

HRESULT MusicPlayer::Done()
{
  if (_ds8Buffer) _ds8Buffer->Release();
  _ds8Buffer=0;
  return 0;

}

DWORD MusicPlayer::GetSafeXFadePos()
{
  DWORD curPos;
  _ds8Buffer->GetCurrentPosition(0,&curPos);
  int curpos=curPos;
  if (curpos>(int)_lastWritePos) curpos-=BUFFER_SIZE;
  curpos+=BUFFER_SIZE/4;
  if (curpos>(int)_lastWritePos) curpos=_lastWritePos;
  if (curpos<0) curpos+=BUFFER_SIZE;
  if (curpos>=BUFFER_SIZE) curpos-=BUFFER_SIZE;
  return curpos;
}

static const float Inv2=0.5;
static const float Snapper=3<<22;

static inline short toInt( float fval )
{
	fval += Snapper;
	return (short)( (*(int *)&fval)&0x007fffff ) - 0x00400000;
}

void MusicPlayer::Close()
{
  if (TryEnterCriticalSection(&_lock)==FALSE)
  {
    DWORD status;
    _ds8Buffer->GetStatus(&status);
    _ds8Buffer->Play(0,0,DSBPLAY_LOOPING);    
    EnterCriticalSection(&_lock);
    if ((status & DSBSTATUS_PLAYING)==0) 
      _ds8Buffer->Stop();    
  }
  
  if (_crossfadebytes==0)
  {
    DWORD xfadepos=GetSafeXFadePos();
    DWORD xfadesz=xfadepos>_lastWritePos?(_lastWritePos+BUFFER_SIZE-xfadepos):(_lastWritePos-xfadepos);
    void *ptr[2];
    DWORD sz[2];
    float ffadesz=(float)xfadesz*0.5f;
    float ffadecnt=0;
    if (xfadesz && _ds8Buffer->Lock(xfadepos,xfadesz,ptr+0,sz+0,ptr+1,sz+1,0)==0)
    {
      for (int i=0;i<2;i++) if (ptr[i])
      {
        for (DWORD j=0;j<sz[i];j+=2)
        {
          short *sample=(short *)((char *)ptr[i]+j);
          sample[0]=toInt(sample[0]-sample[0]*ffadecnt/ffadesz);
          ffadecnt=ffadecnt+1.0f;
          if (ffadecnt>ffadesz) ffadecnt=ffadesz;
        }
      }
      _ds8Buffer->Unlock(ptr[0],sz[0],ptr[1],sz[1]);
    }
    _crossfadebytes=xfadesz;
    _lastWritePos=xfadepos;    
  }
  _opened=false;
  LeaveCriticalSection(&_lock);
}

int MusicPlayer::Write(const char *buf, int len)
{  
  EnterCriticalSection(&_lock);
  if (!_opened) 
  {
    LeaveCriticalSection(&_lock);
    return 1;
  }
  DWORD step=(_stereo?2:1) * (_bit16?2:1);
  void *lockptr[2]={0,0};
  DWORD locksz[2]={0,0};
  void *wrtptr=0;
  DWORD remainspace=0;
  int stage=0;

  while (len>0)
  {
    short sample[2];
    
    if (_bit16)
      if (_stereo)
      {
        sample[0]=*((short *)buf);
        sample[1]=*((short *)buf+1);
      }
      else
      {
        sample[0]=*((short *)buf);
        sample[1]=*((short *)buf);
      }
   else
      if (_stereo)
      {
        sample[0]=(*buf)*256;
        sample[1]=(*(buf+1))*256;
      }
      else
      {
        sample[0]=(*buf)*256;
        sample[1]=(*buf)*256;
      }
    while (remainspace<4) 
    {
      if (stage<1)
      {
        stage++;
        remainspace=locksz[stage];
        wrtptr=lockptr[stage];
      }
      else
      {
        if (lockptr[0])
        {
          _ds8Buffer->Unlock(lockptr[0],locksz[0],lockptr[1],locksz[1]);
          _lastWritePos+=LOCK_GRANUALITY;
          while (_lastWritePos>=BUFFER_SIZE) _lastWritePos-=BUFFER_SIZE;
        }
        DWORD playCursor;
        _ds8Buffer->GetCurrentPosition(&playCursor,0);
        if (playCursor<_lastWritePos) playCursor+=BUFFER_SIZE;
        while (playCursor-_lastWritePos<LOCK_GRANUALITY)
        {
          Sleep(200);
          _ds8Buffer->GetCurrentPosition(&playCursor,0);
          if (playCursor<_lastWritePos) playCursor+=BUFFER_SIZE;
        }
        HRESULT res=_ds8Buffer->Lock(_lastWritePos,LOCK_GRANUALITY,lockptr+0,locksz+0,lockptr+1,locksz+1,0);
        assert(res==0);
        stage=0;
        remainspace=locksz[stage];
        wrtptr=lockptr[stage];
      }        
    }
    if (_crossfadebytes)
    {
      short *data=(short *)wrtptr;
      int32_t a=data[0]+sample[0];
      if (a<-32767) a=-32767;
      if (a>32767) a=32767;
      data[0]=(short)a;
      a=data[1]+sample[1];
      if (a<-32767) a=-32767;
      if (a>32767) a=32767;
      data[1]=(short)a;
      if (_crossfadebytes<4) _crossfadebytes=0;else _crossfadebytes-=4;
    }
    else
      memcpy(wrtptr,sample,4);
    wrtptr=(void *)((char *)wrtptr+4);
    remainspace-=4;
    _minorpos+=_speed;
    while (_minorpos>=1024)
    {
      buf+=step;
      len-=step;
      _minorpos-=1024;
    }
  }
  if (stage==0) {locksz[1]=0;locksz[0]-=remainspace;}
  else {locksz[1]-=remainspace;}
  _ds8Buffer->Unlock(lockptr[0],locksz[0],lockptr[1],locksz[1]);
  _lastWritePos+=locksz[0]+locksz[1];
  while (_lastWritePos>=BUFFER_SIZE) _lastWritePos-=BUFFER_SIZE;
  LeaveCriticalSection(&_lock);
  return 0;
}

int MusicPlayer::CanWrite()
{
  return LOCK_GRANUALITY;
}
int MusicPlayer::IsPlaying()
{
  return 0;
}

int MusicPlayer::Pause(int pause)
{
  int lastState=_paused?1:0;
  if (pause)
    {if (!_paused) {_ds8Buffer->Stop();_paused=true;}}
  else
    {if (_paused) {_ds8Buffer->Play(0,0,DSBPLAY_LOOPING);_paused=false;}}
  return lastState;
}


void MusicPlayer::SetVolume(int volume)
{
  if (volume<0) return;
  _ds8Buffer->SetVolume(_linvoltable[volume]);
  if (volume==0) 
    Pause(1);
  else 
    Pause(0);
}

MusDecoder::MusDecoder()
{
  _playing=false;
  _thread=0;
  _stop=0;
}

MusDecoder::~MusDecoder()
{
  Stop();
}

void MusDecoder::AttachOutput(IWAOutput *o)
{
  if (o) o->AddRef();
  if (_output) _output->Release();
  _output=o;
}

bool MusDecoder::Play(const char *filename)
{
  DWORD res=0;
  if (filename[0]=='?') 
  {
    if (_output->Open(44100,1,8,-1,-1)<0)
    {
      return false;
    }
    _stop=false;
    _playing=true;
    _thread=CreateThread(0,0,MusDecoder::StartSilentWritter,this,0,&res);
    return true;
  }
  _file=CreateFile(filename,GENERIC_READ,0,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
  if (_file==0) return false;
  ReadFile(_file,&_header,sizeof(_header),&res,0);
  if (res!=sizeof(_header)) {CloseHandle(_file);return false;}
  if (_output->Open(_header.freq,_header.channels,16,-1,-1)<0)
  {
    CloseHandle(_file);return false;
  }
  _stop=false;
  _playing=true;
  _thread=CreateThread(0,0,MusDecoder::StartMusDecoder,this,0,&res);
  return true;
}

void MusDecoder::Stop()
{
  if (_thread)
  {
    _stop=true;
    _output->Pause(0);
    WaitForSingleObject(_thread,INFINITE);
    CloseHandle(_thread);
    CloseHandle(_file);
    _output->Close();
    _file=0;
    _thread=0;
  }
}

DWORD WINAPI MusDecoder::StartMusDecoder(LPVOID data)
{
  MusDecoder *self=reinterpret_cast<MusDecoder *>(data);
  return self->MusDecodingThread();
}



UINT MusDecoder::MusDecodingThread()
{
  int32_t blocksRemain=_header.blocks;
  char *packbuf=0;
  short *unpackbuf=0;
  int32_t packbufsz=0;
  int32_t unpackbufsz=0;
  for (int i=0;i<blocksRemain;i++)
  {
    if (_stop) break;
    int32_t packedSize;
    int32_t unpackedSize;
    DWORD bytesread;
    if (ReadFile(_file,&packedSize,sizeof(packedSize),&bytesread,NULL)==FALSE) break;
    if (ReadFile(_file,&unpackedSize,sizeof(unpackedSize),&bytesread,NULL)==FALSE) break;
    if (packbufsz<packedSize)
    {
      free(packbuf);
      packbuf=(char *)malloc(packedSize);
      packbufsz=packedSize;
    }
    if (unpackbufsz<unpackedSize || unpackbuf==0)
    {
      free(unpackbuf);
      unpackbuf=(short *)malloc(unpackedSize);
      unpackbufsz=unpackedSize;
    }
    if (ReadFile(_file,packbuf,packedSize,&bytesread,0)==FALSE) break;
    if (packedSize!=bytesread) break;
    short accum[2]={0,0};
    int channel=0;
    for (int i=0;i<packedSize;i++)
    {
       short val=accum[channel]+_header.ampltable[packbuf[i]];
       accum[channel]=val;
       if (packbuf[i]==0)  //pridano jako provizorni reseni pro korekci chyby komprimacniho programu
          {
          val-=31767;
          }
       channel++;
       if (channel>=_header.channels) channel=0;
       unpackbuf[i]=val;
    }
    _output->Write(reinterpret_cast<char *>(unpackbuf),unpackedSize);
  }
  _playing=false;
  return 0;
}

DWORD WINAPI MusDecoder::StartSilentWritter(LPVOID data)
{
  MusDecoder *self=reinterpret_cast<MusDecoder *>(data);
  return self->SilentWritterThread();
}

UINT MusDecoder::SilentWritterThread()
{
  char empty[1024];
  memset(empty,0,sizeof(empty));
  for (int i=0;i<1024;i++)
  {
    if (_stop) break;
    _output->Write(empty,1024);
  }
  _playing=false;
  return 0;
  
}

WinAmpDecoder::WinAmpDecoder()
{
  _currPlugin=0;
}

WinAmpDecoder::~WinAmpDecoder()
{
  Stop();
}

void WinAmpDecoder::AttachOutput(IWAOutput *o)
{
  if (o) o->AddRef();
  if (_output) _output->Release();
  _output=o;
}

bool WinAmpDecoder::Play(const char *filename)
{
  Stop();
  _currPlugin=SelectBestPlugin(filename);
  if (_currPlugin==0) return false;
  _currPlugin->AttachOutput(_output);
  if (_currPlugin->Play(filename)!=_currPlugin->errOk)
  {
    _currPlugin=0;
    return false;
  }  
  int playtm=_currPlugin->GetOutputTime();
  int nexttm=playtm;
  int c=0;
  MusicPlayer *q=static_cast<MusicPlayer *>(_output);
  while (!q->IsOpenned() && playtm==nexttm && c<2000) {Sleep(1);c++;nexttm=_currPlugin->GetOutputTime();}
  _nooutput=!q->IsOpenned();
  return true;
}

void WinAmpDecoder::Stop()
{
  if (_currPlugin==0) return;  
  _currPlugin->Stop();
  _currPlugin->AttachOutput(0);
  _currPlugin=0;
}

bool WinAmpDecoder::IsPlaying()
{
  if (_currPlugin==0) return false;
  return !_currPlugin->IsFinished();
}

void WinAmpDecoder::SetVolume(int volume, int main)
{  
  _currPlugin->SetVolume(volume);
}