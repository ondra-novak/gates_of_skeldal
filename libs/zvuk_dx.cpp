#define INITGUID
#include <skeldal_win.h>
#include <malloc.h>
#include <debug.h>
#include <stdio.h>
#define DWORD_PTR DWORD *
#include <dsound.h>
#include "types.h"
#include "zvuk.h"
#include <math.h>
extern "C" {
#include <memman.h>
  }
#define MAXCHANNELS 32
#define TIMEIDLESTOPCHANNEL 250
#define DEFAULTBUFFERSIZE (256*1024)
#define FADELENGTH 3000
#define MUS_ERRORWAIT 5000
#include "Music.h"


static int cur_device=0;
static int cur_mixfreq=0;
static bool swap_chans=false;

class CheckRes
  {
  public:
   HRESULT operator=(HRESULT other)
     {
     if (other==0) return 0;
     char buff[256];
     sprintf(buff,"DirectSound error HRESULT %08X, code %d",other,other & 0xFFFF);
     int id=MessageBox(NULL,buff,NULL,MB_SYSTEMMODAL|MB_ABORTRETRYIGNORE);
     if (id==IDRETRY) 
       {
       __asm int 3;
       return other;
       }
     if (id==IDIGNORE) return other;
     exit(-1);
     }
   };

     


  
static CheckRes hres;
static HWND hWndDS8=NULL;
static IDirectSound8 *ds8=NULL;
static IDirectSoundBuffer *ds_primary;
static WAVEFORMATEX waveformat;
static int gfx_volume=255;
static int music_volume=127;
static int glob_volume=255;
static int linvoltable[256];
static MusicPlayer GMusicPlayer;
static MusDecoder GMusDecoder;
static WinAmpDecoder GWinAmpPlayer;
static IDecoder *GCurrentDecoder=&GMusDecoder;


class SoundChannelInfo
  {
  public:
  CRITICAL_SECTION sect;
  char *play_pos, *start_loop, *end_loop;
  IDirectSoundBuffer8 *buffer;
  unsigned long chantype;
  unsigned long idsPlayPos;
  unsigned long idsWritePos;
  unsigned long idsBuffSize;
  long volume;    //volume for DS
  long pan;       //pan for DS
  int volleft;    //left volume for skeldal
  int volright;   //right volume for skeldal
  unsigned long preload;
  unsigned long stopTime; //time, when buffer reached end. For calculating idle time
  unsigned long startTime; //time, when buffer started. For calculating idle time, use 0 to sticky buffer
  SoundChannelInfo() 
    {
    buffer=NULL;
    play_pos=start_loop=end_loop=NULL;
    InitializeCriticalSection(&sect);
    pan=DSBPAN_CENTER;
    volume=DSBVOLUME_MAX;
    }
  ~SoundChannelInfo() {if (buffer) buffer->Release();DeleteCriticalSection(&sect);}
  unsigned long CalculateLockSize();
  void ChannelMaintaince();     
  bool IsPlaying() {return play_pos!=NULL;}  
//  bool IsFree(char type) {return play_pos==NULL || (type==chantype && play_pos==end_loop);}
  void Reset();
  void InitChannel(char type, char *ppos, char *bloop,char *eloop, int freq);
  void Lock() {EnterCriticalSection(&sect);}
  void Unlock() {LeaveCriticalSection(&sect);}
  bool TryLock() {return TryEnterCriticalSection(&sect)!=FALSE;}
  void SetVolume(long vol) 
    {
    Lock();
    if (volume!=vol) if (buffer && play_pos ) hres=buffer->SetVolume(vol);
    volume=vol;    
    Unlock();
    }
  void SetPan(long p) 
    {
    Lock();
    if (pan!=p) if (buffer && play_pos ) hres=buffer->SetPan(p);
    pan=p;
    Unlock();
    }
  void Mute() 
    {
    Lock();
    Stop();
    Unlock();
    }
  void Stop()
	{
	if (buffer)
	  buffer->Stop();
	play_pos=NULL;
	}
    
  void BreakLoop() {start_loop=end_loop;}
  void BreakLoopEx(char *end_sample) {start_loop=end_loop=end_sample;}
  };

static bool shutMaintaince=false;
static SoundChannelInfo channels[MAXCHANNELS];
static HANDLE maintainceThread;

ULONG __stdcall MaintainceThread(void *);
  
static void DSStart(int mixfreq)
  {
  DSBUFFERDESC desc; 

  for (int i=1;i<256;i++) 
    {
    double rang=pow(i/255.0,15);
    linvoltable[i]=100*log(rang);
    
    }
  linvoltable[0]=-10000;

  hres=DirectSoundCreate8(&DSDEVID_DefaultPlayback,&ds8,NULL);
  if (hWndDS8) hres=ds8->SetCooperativeLevel(hWndDS8,DSSCL_PRIORITY);
  memset(&desc,0,sizeof(desc));
  desc.dwSize=sizeof(desc);
  desc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME;
  hres=ds8->CreateSoundBuffer(&desc,&ds_primary,NULL); 

  waveformat.cbSize=sizeof(waveformat);
  waveformat.nBlockAlign=4;
  waveformat.nChannels=2;
  waveformat.nSamplesPerSec=mixfreq;
  waveformat.nAvgBytesPerSec=mixfreq*4;
  waveformat.wBitsPerSample=16;
  waveformat.wFormatTag=WAVE_FORMAT_PCM;  

  hres=ds_primary->SetFormat(&waveformat);
  DWORD id;

  shutMaintaince=false;
  maintainceThread=CreateThread(NULL,0,MaintainceThread,NULL,0,&id);
  SetThreadPriority(maintainceThread,THREAD_PRIORITY_HIGHEST);

  GMusicPlayer.InitBuffer(ds8,linvoltable);
  GMusicPlayer.Play();
  } 

void init_winamp_plugins(const char *path)
{
  GWinAmpPlayer.LoadPlugins(path);
}

static void DSStop()
  {  
  GCurrentDecoder->Stop();
  GMusDecoder.Stop();
  GMusicPlayer.Done();
  GWinAmpPlayer.ClearList();
  shutMaintaince=true;
  WaitForSingleObject(maintainceThread,INFINITE);
  CloseHandle(maintainceThread);
  for (int i=0;i<MAXCHANNELS;i++) 
    if (channels[i].buffer) 
      {channels[i].Stop(); channels[i].buffer->Release();channels[i].buffer=NULL;}
  if (ds_primary) ds_primary->Release();
  ds_primary=NULL;
  if (ds8) ds8->Release();   
  ds8=NULL;
  }
  
unsigned long SoundChannelInfo::CalculateLockSize()
  {
  if (buffer==NULL) return 0;
  unsigned long playpos;
  buffer->GetCurrentPosition(NULL,&playpos);
  long diff=(signed)playpos-(signed)idsPlayPos;
  if (diff<0) diff+=idsBuffSize;
  unsigned long wendpos=playpos+preload; 
  if (wendpos>=idsBuffSize) wendpos-=idsBuffSize;
  if (wendpos<idsWritePos && wendpos>playpos) return 0;
  if (wendpos<idsWritePos) wendpos+=idsBuffSize;
  unsigned long sz=(wendpos-idsWritePos+3) & ~3;
  if (sz>idsBuffSize/2) sz=idsBuffSize/2;
  idsPlayPos=playpos;
  return sz;
  }

void SoundChannelInfo::ChannelMaintaince()
  {
  Lock();
  if (play_pos!=NULL)
    {
    if (play_pos!=end_loop) stopTime=GetTickCount();
    unsigned long lockSize=CalculateLockSize();
    if (lockSize)
      {
  //    printf("%8d\r",lockSize);
      void *audioptrs[2];
      unsigned long sizes[2];
      hres=buffer->Lock(idsWritePos,lockSize,audioptrs,sizes,audioptrs+1,sizes+1,0);
      for (int i=0;i<2 && audioptrs[i];i++)
        {
        char *wrt=(char *)audioptrs[i];
        for (unsigned long j=0;j<sizes[i];j++) if (play_pos!=end_loop) 
          {
          *wrt++=*play_pos++;
          if (play_pos==end_loop) play_pos=start_loop;               
          }
        else
          {
          if (chantype & 0x1) *wrt++=0x80; else *wrt++=0;
          }
        }      
      hres=buffer->Unlock(audioptrs[0],sizes[0],audioptrs[1],sizes[1]);      
      idsWritePos+=lockSize;
      if (idsWritePos>=idsBuffSize) idsWritePos-=idsBuffSize;
      }
    if (play_pos==end_loop && GetTickCount()-stopTime>TIMEIDLESTOPCHANNEL)
        {
        Stop();        
        }           
    }
  Unlock();
  }
  
static void MixMaintaince()
  {  
  for (int i=0;i<MAXCHANNELS;i++) channels[i].ChannelMaintaince();   
  }

static ULONG __stdcall MaintainceThread(void *)
  {
  while (!shutMaintaince) {MixMaintaince();Sleep(50);}
  return 0;
  }

void SoundChannelInfo::Reset()
  {
  Lock();
  if (buffer) buffer->Release();
  buffer=NULL;
  Unlock();
  }

void SoundChannelInfo::InitChannel(char type, char *ppos, char *bloop,char *eloop, int freq)
  {
  Lock();
  unsigned long newchantype=type+freq*4;
  if (chantype!=newchantype || buffer==NULL)
    {
    Reset();
    DSBUFFERDESC desc; 
    WAVEFORMATEX wfex;
    wfex.cbSize=sizeof(wfex);
    wfex.wFormatTag=WAVE_FORMAT_PCM;
    wfex.wBitsPerSample=type==1?8:16;
    wfex.nSamplesPerSec=freq;
    wfex.nChannels=1;
    wfex.nBlockAlign=type==1?1:2;
    wfex.nAvgBytesPerSec=wfex.nBlockAlign*wfex.nSamplesPerSec;    

    memset(&desc,0,sizeof(desc));
    desc.dwSize=sizeof(desc);
    desc.dwFlags=DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_GETCURRENTPOSITION2;
    desc.dwBufferBytes=DEFAULTBUFFERSIZE;
    desc.lpwfxFormat=&wfex;
    
    preload=wfex.nAvgBytesPerSec/5;

    IDirectSoundBuffer *bufold;
    
    hres=ds8->CreateSoundBuffer(&desc,&bufold,NULL);
    hres=bufold->QueryInterface(IID_IDirectSoundBuffer8,(void **)&buffer);
    bufold->Release();

    idsPlayPos=DEFAULTBUFFERSIZE-cur_mixfreq;
    idsWritePos=0;
    idsBuffSize=DEFAULTBUFFERSIZE;

    play_pos=ppos;
    start_loop=bloop;
    end_loop=eloop;

    chantype=newchantype;

    hres=buffer->SetVolume(volume);
    hres=buffer->SetPan(pan);

    ChannelMaintaince();
    
    hres=buffer->Play(0,0,DSBPLAY_LOOPING);
    stopTime=startTime=GetTickCount();   
    }
  else
    {
    buffer->Stop();
    idsBuffSize=DEFAULTBUFFERSIZE;
    buffer->SetCurrentPosition(0);
    buffer->GetCurrentPosition(&idsPlayPos,&idsWritePos);
    hres=buffer->SetVolume(volume);
    hres=buffer->SetPan(pan);
    play_pos=ppos;
    start_loop=bloop;
    end_loop=eloop;
    ChannelMaintaince();
    buffer->Play(0,0,DSBPLAY_LOOPING);
    stopTime=startTime=GetTickCount();   
    }
  Unlock();
  }

/*  
static int FindFreeChannel(char type)
  {
  int older=0;
  DWORD m_age=0;
  DWORD timval=GetTickCount();
  for (int i=0;i<MAXCHANNELS;i++) if (!channels[i].IsFree(char type)) return i;
    {
    DWORD age==timval-channels[i].startTime;
    if (age>m_age) {older=i;m_age=age;}    
    }
  return older;
  }
  */


extern "C"
  {
int bvolume;      //background volume
void (*konec_skladby)(char **jmeno)=NULL; //pointer to function to notify that end of song has been reached

void DSReportWindowCreation(HWND hWindow)
  {
  hWndDS8=hWindow;
  if (ds8!=NULL) hres=ds8->SetCooperativeLevel(hWndDS8,DSSCL_PRIORITY);
  }


int sound_detect(int *dev,int *port,int *dma, int *irq)
  {
  *dev=DEV_DIRECTSOUND;
  *port=0;
  *dma=0;
  *irq=0;
  return 0;
  }
  
void set_mixing_device(int mix_dev,int mix_freq,...)
  {
  cur_device=mix_dev;
  cur_mixfreq=mix_freq;
  }

char start_mixing()
  {
  if (cur_device!=DEV_DIRECTSOUND) 
    {
    MessageBox(hWndDS8,"Invalid sound device! Check SKELDAL.INI. Only device 9 (DirectSound) can be used.",NULL,MB_SYSTEMMODAL);
    exit(1);
    }
  if (cur_mixfreq==0) return FALSE;
  DSStart(cur_mixfreq);
  return TRUE;
  } 
void stop_mixing()
  {
  DSStop();
  }
 
void play_sample(int channel,void *sample,long size,long lstart,long sfreq,int type)
  {  
  char *start=(char *)sample;
  channels[channel].InitChannel(type,start,start+lstart,start+size,sfreq);
  }

void set_channel_volume(int channel,int left,int right)
  {
  if (left>32767) left=32767;
  if (left<0) left=0;
  if (right>32767) right=32767;
  if (right<0) right=0;
  int volleft=linvoltable[(left>>7)*gfx_volume/255];
  int volright=linvoltable[(right>>7)*gfx_volume/255];
  int volsum=__max(volleft,volright);
  channels[channel].SetVolume(volsum);
  channels[channel].SetPan(swap_chans?(volright-volleft):(volleft-volright));
  channels[channel].volleft=left;
  channels[channel].volright=right;
  }

char get_channel_state(int channel)
  {
  return channels[channel].IsPlaying()==true;
  }

void get_channel_volume(int channel,int *left,int *right)
  {
  if (left) *left=channels[channel].volleft;
  if (right) *right=channels[channel].volright;
  }

void mute_channel(int channel)
  {
  channels[channel].Mute();
  }

void chan_break_loop(int channel)
  {
  channels[channel].BreakLoop();
  }

void chan_break_ext(int channel,void *org_sample,long size_sample) //zrusi loop s moznosti dohrat zvu
  {
  char *end_sample=(char *)org_sample+size_sample;
  channels[channel].BreakLoopEx(end_sample);
  }

char set_snd_effect(int funct,int data)
  {
  switch (funct)
    {
    case SND_PING: return 1;
    case SND_SWAP: swap_chans=(data & 1)!=0;return 1;
    case SND_GFX: gfx_volume=data;return 1;
    case SND_MUSIC: GCurrentDecoder->SetVolume(music_volume=data,get_snd_effect(SND_GVOLUME));return 1;
    case SND_GVOLUME: hres=ds_primary->SetVolume(linvoltable[glob_volume=data]);return 1;
    default: return 0;
    }
  }

int  get_snd_effect(int funct)
  {
  switch (funct)
    {
    case SND_PING: return 1;
    case SND_SWAP: return swap_chans;
    case SND_GFX: return gfx_volume;
    case SND_MUSIC: return music_volume;
    case SND_GVOLUME: return glob_volume;
    default: return 0;
    }
  
  }

char check_snd_effect(int funct)
  {
  switch (funct)
    {
    case SND_PING: 
    case SND_SWAP:
    case SND_MUSIC:
    case SND_GVOLUME: 
    case SND_GFX: return 1;
    default: return 0;
    }
  }




static DWORD Mus_buffSize;
/*
static HANDLE music_file=NULL;
static HANDLE next_music_file=NULL;
static bool fading=false;
static DWORD fadetime;
static DWORD Mus_lastWritePos;
static DWORD Mus_nextBlockSize;
static DWORD Mus_nextBlockRead;
static DWORD Mus_errorWait=0;
static DWORD Mus_silentPlay=0;

#pragma pack (1)

struct MusFile
  {
  short channels;
  long  freq;
  long  ssize;
  long  blocks;
  long  reserved1;
  long  reserved2;
  short ampltable[256];
  };
  
static MusFile curMus;

#pragma pack()




static char OpenMus(HANDLE music_file)
  {
  DWORD bytesread;

  SetFilePointer(music_file,0,NULL,FILE_BEGIN);

  if (ReadFile(music_file,&curMus,sizeof(curMus),&bytesread,NULL)==TRUE && bytesread==sizeof(curMus))    
    {	
    if (ReadFile(music_file,&Mus_nextBlockRead,sizeof(Mus_nextBlockRead),&bytesread,NULL)==FALSE) return 0;
    if (ReadFile(music_file,&Mus_nextBlockSize,sizeof(Mus_nextBlockSize),&bytesread,NULL)==FALSE) return 0;
		
    
    if (ds_music!=NULL) ds_music->Release();
    
    WAVEFORMATEX wfex;
    wfex.cbSize=sizeof(wfex);
    wfex.wBitsPerSample=16;
    wfex.nBlockAlign=2*curMus.channels;
    wfex.nSamplesPerSec=curMus.freq;
    wfex.nAvgBytesPerSec=curMus.freq*wfex.nBlockAlign;
    wfex.wFormatTag=WAVE_FORMAT_PCM;
    wfex.nChannels=curMus.channels;
    
    DSBUFFERDESC desc;
    desc.dwSize=sizeof(desc);
    desc.dwBufferBytes=Mus_buffSize=wfex.nAvgBytesPerSec*4;
    desc.dwReserved=0;
    desc.dwFlags=DSBCAPS_CTRLVOLUME ;
    desc.lpwfxFormat=&wfex;

    IDirectSoundBuffer *dsbf;

    hres=ds8->CreateSoundBuffer(&desc,&dsbf,NULL);
    hres=dsbf->QueryInterface(IID_IDirectSoundBuffer8,(void **)&ds_music);
    dsbf->Release();    
    
    void *ptr;
    DWORD size;

    ds_music->Lock(0,0,&ptr,&size,NULL,NULL,DSBLOCK_ENTIREBUFFER);
    memset(ptr,0,size);
    ds_music->Unlock(ptr,size,NULL,NULL);
    ds_music->SetVolume(linvoltable[music_volume]);
    ds_music->Play(0,0,DSBPLAY_LOOPING);    
    fadetime=0;
    Mus_lastWritePos=wfex.nAvgBytesPerSec;
    Mus_silentPlay=0;

    return 1;
    }
  return 0;
  }

static void PrepareMusFile(const char *filename)
  {
  CloseHandle(next_music_file);
  next_music_file=CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,
                 NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                 NULL);
  if (next_music_file==INVALID_HANDLE_VALUE)
    next_music_file=NULL;
  }

static char music_decompres_block()
  {
  DWORD bytesread;
  char *data=(char *)alloca(Mus_nextBlockRead);
  short accum[2]={0,0};
  char curchan=0;
  
  DWORD lockSizes[2];
  void *lockPtrs[2];

  if (ReadFile(music_file,data,Mus_nextBlockRead,&bytesread,NULL)==FALSE) return 0;

  ds_music->Lock(Mus_lastWritePos,Mus_nextBlockSize,lockPtrs,lockSizes,lockPtrs+1,lockSizes+1,0);
  for (int j=0;j<2;j++)    
    {
    short *target=(short *)lockPtrs[j];
    for (DWORD i=0;i<lockSizes[j];i+=2)
      {
      
      short val=accum[curchan]+curMus.ampltable[*data++];
      accum[curchan]=val;
      if (data[-1]==0)  //pridano jako provizorni reseni pro korekci chyby komprimacniho programu
          {
          val-=31767;
          }
      if (fadetime)
        {
        long ftime=FADELENGTH-(GetTickCount()-fadetime);
        if (ftime<0) ftime=0;
        float mul=(float)ftime*(float)ftime/(float)(FADELENGTH*FADELENGTH);
        val=(short)(val*mul);        
        }
      *target++=val;
      curchan++;
      if (curchan>=curMus.channels) curchan=0;
      }
    }
  ds_music->Unlock(lockPtrs[0],lockSizes[0],lockPtrs[1],lockSizes[1]);
  Mus_lastWritePos+=Mus_nextBlockSize;
  if (Mus_lastWritePos>=Mus_buffSize) Mus_lastWritePos-=Mus_buffSize;
  return 1;
  }

static char music_silent_play()
  {
  DWORD lockSizes[2];
  void *lockPtrs[2];

  ds_music->Lock(Mus_lastWritePos,Mus_nextBlockSize,lockPtrs,lockSizes,lockPtrs+1,lockSizes+1,0);
  for (int j=0;j<2;j++)    
    {
    short *target=(short *)lockPtrs[j];
    for (DWORD i=0;i<lockSizes[j];i+=2)
      {
      *target++=0;
      }
    }
  ds_music->Unlock(lockPtrs[0],lockSizes[0],lockPtrs[1],lockSizes[1]);
  Mus_lastWritePos+=Mus_nextBlockSize;
  if (Mus_lastWritePos>=Mus_buffSize) Mus_lastWritePos-=Mus_buffSize;
  return 1;
  }


void fade_music()
  {
  if (fadetime==0) fadetime=music_file?GetTickCount():0;
  }


static int mix_back_sound_worker()
  {
  DWORD bytesread;
  if (music_file==NULL) return -2;
  if (ds_music==NULL) 
    if (OpenMus(music_file)==0) return -2;
  DWORD play;
  ds_music->GetCurrentPosition(&play,NULL);
  while (true)
    {
    if (Mus_lastWritePos<=play) 
      {
      if (Mus_lastWritePos+Mus_nextBlockSize>=play) return 0;
      }
    else
      {
      if (Mus_lastWritePos+Mus_nextBlockSize>play+Mus_buffSize) return 0;
      } 
    if (Mus_silentPlay)
      {
      if (Mus_silentPlay>GetTickCount())
        {
        music_silent_play();
        return 0;
        }
      else
        return -2;
      }

    if (music_decompres_block()==0) return -2;
    curMus.blocks--;
    if (curMus.blocks<0) 
      {
      Mus_silentPlay=GetTickCount()+4000;
      return 0;      
      }
  if (ReadFile(music_file,&Mus_nextBlockRead,sizeof(Mus_nextBlockRead),&bytesread,NULL)==FALSE) return -2;
  if (ReadFile(music_file,&Mus_nextBlockSize,sizeof(Mus_nextBlockSize),&bytesread,NULL)==FALSE) return -2;
    if (fadetime && fadetime+FADELENGTH<GetTickCount())       
      {
      Mus_silentPlay=GetTickCount()+4000;
      return 0;      
      }

    }
  Mus_errorWait=0;
  return 0;
  }

  */
int mix_back_sound(int synchro)
  {
    if (GCurrentDecoder->IsPlaying()) return 0;
    char *next_music;

    konec_skladby(&next_music);
    change_music(next_music);
    return 0;
  }

void change_music(const char *mus_filename)
  {
    if (GCurrentDecoder->NotUsingOutput()) GMusDecoder.Stop();
    GCurrentDecoder->Stop();    
    if (mus_filename==0) 
    {
      mus_filename="?";
      GCurrentDecoder=&GMusDecoder;
    }
    else
    {
      const char *c=strrchr(mus_filename,'.');
      if (c!=0 && stricmp(c,".mus")==0)      
        GCurrentDecoder=&GMusDecoder;
      else
        GCurrentDecoder=&GWinAmpPlayer;
    }
    GCurrentDecoder->AttachOutput(&GMusicPlayer);    
    if (GCurrentDecoder->Play(mus_filename)==false) change_music(0);
    GCurrentDecoder->SetVolume(music_volume,get_snd_effect(SND_GVOLUME));
    if (GCurrentDecoder->NotUsingOutput())
      GMusDecoder.Play("?");
  }

  
int get_timer_value()
  {
  return GetTickCount()/TIMERSPEED
  }
char *device_name(int device)
  {
  if (device!=DEV_DIRECTSOUND) return "Unknown device!";
  else return "DirectSound 8";
  }
void force_music_volume(int volume)
  {
  }
void set_backsnd_freq(int bfreq)
  {
  }

void *PrepareVideoSound(int mixfreq, int buffsize)
  {
    WAVEFORMATEX wfex;
    wfex.cbSize=sizeof(wfex);
    wfex.wBitsPerSample=16;
    wfex.nBlockAlign=4;
    wfex.nSamplesPerSec=mixfreq;
    wfex.nAvgBytesPerSec=mixfreq*wfex.nBlockAlign;
    wfex.wFormatTag=WAVE_FORMAT_PCM;
    wfex.nChannels=2;
    
    DSBUFFERDESC desc;
    desc.dwSize=sizeof(desc);
    desc.dwBufferBytes=Mus_buffSize=buffsize;
    desc.dwReserved=0;
    desc.dwFlags=DSBCAPS_CTRLVOLUME ;
    desc.lpwfxFormat=&wfex;

    IDirectSoundBuffer *dsbf;
	IDirectSoundBuffer8 *ds_music;

    hres=ds8->CreateSoundBuffer(&desc,&dsbf,NULL);
    hres=dsbf->QueryInterface(IID_IDirectSoundBuffer8,(void **)&ds_music);
    dsbf->Release();    
    
    void *ptr;
    DWORD size;

    ds_music->Lock(0,0,&ptr,&size,NULL,NULL,DSBLOCK_ENTIREBUFFER);
    memset(ptr,0,size);
    ds_music->Unlock(ptr,size,NULL,NULL);
    ds_music->SetVolume(0);
    ds_music->Play(0,0,DSBPLAY_LOOPING);    
	return (void *)ds_music;
  }

char LoadNextVideoFrame(void *buffer, char *data, int size, short *xlat,short *accnums, long *writepos)
  {
  IDirectSoundBuffer8 *ds_music=(IDirectSoundBuffer8 *)buffer;
  DSBCAPS caps;
  caps.dwSize=sizeof(caps);
  ds_music->GetCaps(&caps);
  DWORD play;
  ds_music->GetCurrentPosition(&play,NULL);
  long remain=play-*writepos;
  if (remain<0) remain+=caps.dwBufferBytes;
  if (remain<size*2) return 0;
  char curchan=0;

  DWORD lockSizes[2];
  void *lockPtrs[2];

  ds_music->Lock(*writepos,size*2,lockPtrs,lockSizes,lockPtrs+1,lockSizes+1,0);
  for (int j=0;j<2;j++)    
    {
    short *target=(short *)lockPtrs[j];
    for (DWORD i=0;i<lockSizes[j];i+=2)
      {      
      int val=accnums[curchan]+xlat[*data++];
	  if (val>32767) 
		{accnums[curchan]-=val-32768;val=32767;}
	  if (val<-32767) 
		{accnums[curchan]-=val+32768;val=-32767;}
      accnums[curchan]=val;
/*      if (data[-1]==0)  //pridano jako provizorni reseni pro korekci chyby komprimacniho programu
          {
          val-=31767;
          }*/
      *target++=val;
      curchan++;
      if (curchan>=2) curchan=0;
      }
    }
  ds_music->Unlock(lockPtrs[0],lockSizes[0],lockPtrs[1],lockSizes[1]);
  writepos[0]+=size*2;
  if (writepos[0]>caps.dwBufferBytes) writepos[0]-=caps.dwBufferBytes;
  return 1;
  }


void DoneVideoSound(void *buffer)
  {
  IDirectSoundBuffer8 *ds_music=(IDirectSoundBuffer8 *)buffer;
  ds_music->Stop();
  ds_music->Release();
  }

}

