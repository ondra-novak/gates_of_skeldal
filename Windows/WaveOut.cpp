// WaveOut.cpp: implementation of the WaveOut class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WaveOut.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WaveOut::WaveOut()
{
_device=0;
}

WaveOut::~WaveOut()
{
if (_device) Close();
}


int WaveOut::Open(int samplerate, int numchannels, int bitspersamp, int bufferlenms, int prebufferms)
{
  MSG msg;
  PeekMessage(&msg,0,0,0,0); //ensure message queue created
  _paused=false;
  _wrpos=0;
  _pending=0;
  memset(_buffers,0,sizeof(_buffers));
  _played=0;
  _written=0;

  WAVEFORMATEX format;
  format.cbSize=sizeof(format);
  format.nChannels=numchannels;
  format.nSamplesPerSec=samplerate;
  format.nAvgBytesPerSec=numchannels*samplerate*(bitspersamp/8);
  format.nBlockAlign=numchannels*(bitspersamp/8);
  format.wBitsPerSample=bitspersamp;
  format.wFormatTag=WAVE_FORMAT_PCM;
  _event=CreateEvent(NULL,FALSE,FALSE,NULL);
  _persec=format.nAvgBytesPerSec;
  MMRESULT res=waveOutOpen(&_device,WAVE_MAPPER,&format,(DWORD)_event,(DWORD)this,CALLBACK_EVENT);
  if (res==MMSYSERR_NOERROR) return 0;  
  return -1;
}

void WaveOut::Close()
{
  Flush(1);
  waveOutClose(_device);
  _device=0;

}

int WaveOut::NextBuffer()
{
  int b=_wrpos;
  printf("Buffers: %5d time: %d\r",_pending,GetOutputTime()/1000);
  if (_buffers[_wrpos]==0 || (_buffers[_wrpos]->dwFlags & WHDR_DONE))
  {
	if (_buffers[_wrpos])
	{
		waveOutUnprepareHeader(_device,_buffers[_wrpos],sizeof(*_buffers[_wrpos]));
		free(_buffers[_wrpos]->lpData);
		_played+=_buffers[_wrpos]->dwBufferLength;
		delete _buffers[_wrpos];
		_buffers[_wrpos]=0;
		_pending--;
	}
	_wrpos++;
	if (_wrpos>=MAXBUFFERS) _wrpos=0;
	return b;
  }
  return -1;
}


int WaveOut::Write(const char *buf, int len)
{
  int pos=NextBuffer();
  while (pos==-1) 
  {
	WaitForSingleObject(_event,INFINITE);	
	pos=NextBuffer();
  }
  WAVEHDR *hdr=new WAVEHDR;
  memset(hdr,0,sizeof(*hdr));
  hdr->dwBufferLength=len;
  hdr->lpData=(char *)malloc(len);
  memcpy(hdr->lpData,buf,len);
  waveOutPrepareHeader(_device,hdr,sizeof(*hdr));
  waveOutWrite(_device,hdr,sizeof(*hdr));
  _buffers[pos]=hdr;
  _pending++;
  _written+=len;
  return 0;
}

int WaveOut::CanWrite()
{
  return _paused?0:16*1024*1024;
}

int WaveOut::IsPlaying()
{  
  while (_pending)	
  {
	if (NextBuffer()==-1) return 1;
  }
  return 0;
}

int WaveOut::Pause(int pause)
{
  bool waspaused=_paused;
  if (pause)
  {
	if (!waspaused) waveOutPause(_device);
	_paused=true;
  }
  else
  {
	if (waspaused) waveOutRestart(_device);
	_paused=false;
  }
  return waspaused?1:0;
}

void WaveOut::SetVolume(int volume)
{

}

void WaveOut::SetPan(int pan)
{

}


void WaveOut::Flush(int t)
{
  waveOutReset(_device);
  while (_pending) NextBuffer();
}
 

int WaveOut::GetOutputTime()
{
  return (int)((__int64)_played*(__int64)1000/(__int64)_persec);
}

int WaveOut::GetWrittenTime()
{
  return (int)((__int64)_played*(__int64)1000/(__int64)_persec);
}
