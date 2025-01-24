// WStringMemorySingleThread.cpp: implementation of the WStringMemory class.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include <windows.h>
#include "WStringMemory.h"
#include "WStringProxy.h"
#include <assert.h>
#include <stdio.h>
#include <projects/btree/btree.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef _MT	//multithreading
#define WSTRING_MT
#endif

#ifdef WSTRING_MT	  //multithreading


struct WStringMTLock
{
  WStringProxy *_lockProxy;
  LONG _recursionCount;
  DWORD _owner;
  HANDLE _event;
  WStringMTLock(WStringProxy *x):_lockProxy(x) {}
  WStringMTLock():_lockProxy(NULL) {}

  bool operator==(const WStringMTLock &other) const {return _lockProxy==other._lockProxy;}
  bool operator!=(const WStringMTLock &other) const {return _lockProxy!=other._lockProxy;}
  bool operator>=(const WStringMTLock &other) const {return _lockProxy>=other._lockProxy;}
  bool operator<=(const WStringMTLock &other) const {return _lockProxy<=other._lockProxy;}
  bool operator>(const WStringMTLock &other) const {return _lockProxy>other._lockProxy;}
  bool operator<(const WStringMTLock &other) const {return _lockProxy<other._lockProxy;}
  int operator=(int zero) {_lockProxy=NULL;return zero;}
};

static CRITICAL_SECTION GLocker={{0},-1,0,0,0,0};
static HANDLE GLockProxy=NULL;	//event object to notify waiters that somebody unlocks;

static BTree<WStringMTLock> *GLockDB=NULL;  //Lock proxy database

static void exitMT()
{
  DeleteCriticalSection(&GLocker);
}


static void OnStartup()
{ 
  InitializeCriticalSection(&GLocker);
  atexit(exitMT);
}

#define ON_STARTUP_PRIORITY_NORMAL OnStartup
#include <Es/Common/Startup.hpp>

#endif



#define WS_MAXFREELISTS 32
#define WS_FREELISTSTEP 32
#define WS_TOTALMAXFREEKBYTES 256

#define WS_MAXIMUMFASTALLOC (WS_MAXFREELISTS*WS_FREELISTSTEP)

#define WS_MAXFREEBYTES PerSlotMaxAlloc


static size_t PerSlotMaxAlloc=(WS_TOTALMAXFREEKBYTES*1024/WS_MAXFREELISTS);
static WStringProxy *FreeList=NULL;
static void **StringFreeList[WS_MAXFREELISTS];
static size_t StringFreeBytes[WS_MAXFREELISTS];
static bool InitManager=true;


static void InitManagerPointers()
{
  memset(StringFreeList,0,sizeof(StringFreeList));
  memset(StringFreeBytes,0,sizeof(StringFreeBytes));
  InitManager=false;
}

static void *AllocStringBlock(size_t sz)
{
  if (InitManager) InitManagerPointers();
  if (sz>WS_MAXIMUMFASTALLOC) return malloc(sz);
  int pos=(sz+WS_FREELISTSTEP-1)/WS_FREELISTSTEP;
  void **nxt=StringFreeList[pos];
  if (nxt==0) 
  {
    printf("malloc %d\n",pos*WS_FREELISTSTEP);
    return malloc(pos*WS_FREELISTSTEP);
  }
  printf("fast_alloc %d\n",pos*WS_FREELISTSTEP);
  StringFreeList[pos]=(void **)(*nxt);
  StringFreeBytes[pos]-=_msize(nxt);
  return nxt;
}

static void DeallocStringBlock(void *ptr)
{
  size_t sz=_msize(ptr);
  if (sz>WS_MAXIMUMFASTALLOC) {free(ptr);return;}
  int pos=(sz+WS_FREELISTSTEP-1)/WS_FREELISTSTEP;
  if (sz+StringFreeBytes[pos]>WS_MAXFREEBYTES) 
    {
      printf("free %d\n",sz);
      free(ptr);return;
    }
  void **proxy=(void **)ptr;
  *proxy=(void *)StringFreeList[pos];
  StringFreeList[pos]=proxy;
  StringFreeBytes[pos]+=sz;
  printf("fast_free %d\n",sz);
}


static inline void *operator new(size_t alloc,size_t sz)
{
  return AllocStringBlock(sz+alloc);
}

static inline void operator delete(void *p,size_t alloc)
{
  DeallocStringBlock(p);
}

static inline void *operator new(size_t sz,void *ptr)
{
  return ptr;
}

static inline void operator delete(void *ptr,void *p)
{
}

WStringProxy * WStringMemory::AllocString(const wchar_t *text, size_t size)
{
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  assert(size!=0 || text!=0);
  if (size==0) size=wcslen(text);
  WStringProxy *proxy=new((size+1)*sizeof(wchar_t)) WStringProxy(size,0,0);
  wchar_t *alloctext=const_cast<wchar_t *>(proxy->GetStringFromMemBlock());
  if (text)	wcsncpy(alloctext,text,size);  
  alloctext[size]=0;
  if (proxy->_redirect==0) 
    {
      proxy->_redirect=alloctext;
      proxy->_blockData2=1;
    }
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
  return proxy;
}

WStringProxy * WStringMemory::AllocProxy(const WStringProxy &templateProxy)
{
  WStringProxy * res;
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  if (FreeList==NULL) res=new WStringProxy(templateProxy);
  else
  {
	WStringProxy *alloc=FreeList;
	FreeList=alloc->_baseString;
	res=new((void *)alloc) WStringProxy(templateProxy);
  }
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
  return res;
}

void WStringMemory::FreeProxy(WStringProxy *proxy)
{
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  if (proxy->_operation==proxy->OpMemBlck && !(proxy->_blockData2==0 && proxy->_redirect!=NULL)) 
  {
    if (proxy->_blockData2==2) UnshareString(proxy);
      DeallocStringBlock(proxy);
  }
  else
  {
	proxy->~WStringProxy();
	proxy->_baseString=FreeList;
	FreeList=proxy;
  }
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
}

#ifdef WSTRING_MT

void WStringMemory::LockProxy( WStringProxy *proxy)
{
nextTry:
  EnterCriticalSection(&GLocker);
  WStringMTLock srch(proxy),*found;
  if (GLockDB==NULL) GLockDB=new BTree<WStringMTLock>(16);
  found=GLockDB->Find(srch);
  if (found==NULL)
  {
	srch._event=NULL;
	srch._owner=GetCurrentThreadId();
	srch._recursionCount=1;
	GLockDB->Add(srch);
  }
  else
  {	
	if (found->_owner!=GetCurrentThreadId())
	{
	  HANDLE w=found->_event;
	  if (w==0) {w=found->_event=CreateEvent(NULL,TRUE,FALSE,NULL);}
	  LeaveCriticalSection(&GLocker);			//leave section
	  WaitForSingleObject(w,INFINITE);
	  goto nextTry;
	}
	else
	{
	  found->_recursionCount++;
	}
  }
  LeaveCriticalSection(&GLocker);			//leave section
}

void WStringMemory::UnlockProxy( WStringProxy *proxy)
{
  EnterCriticalSection(&GLocker);  
  WStringMTLock srch(proxy),*found;
  found=GLockDB->Find(srch);
  if (found) 
  {	
	if (--found->_recursionCount==0)
	{
	  if (found->_event!=NULL) 
	  {
		SetEvent(found->_event);
		CloseHandle(found->_event);
	  }
	  GLockDB->Remove(*found);
	}
  }
  LeaveCriticalSection(&GLocker);		
}


void WStringMemory::AddRefProxy(WStringProxy *proxy)
{  
  InterlockedIncrement(reinterpret_cast<LONG *>(&proxy->_refCount));
}

bool WStringMemory::ReleaseRefProxy(WStringProxy *proxy)
{
  LONG res=InterlockedDecrement(reinterpret_cast<LONG *>(&proxy->_refCount));
  if (res<0) res=InterlockedIncrement(reinterpret_cast<LONG *>(&proxy->_refCount));  
  return res==0;
}

#else
void WStringMemory::LockProxy( WStringProxy *proxy)
{
  //not needed in single thread environment
}

void WStringMemory::UnlockProxy( WStringProxy *proxy)
{
  //not needed in single thread environment
}


void WStringMemory::AddRefProxy(WStringProxy *proxy)
{
  //no special handling in single thread environment
  ++proxy->_refCount;
}

bool WStringMemory::ReleaseRefProxy(WStringProxy *proxy)
{
  //no special handling in single thread environment
  if (proxy->_refCount) --proxy->_refCount;
  return proxy->_refCount==0;
}

#endif
void WStringMemory::FreeExtra()
{
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  while (FreeList)
  {
    void *proxy=FreeList;
    FreeList=FreeList->_baseString;
    free(proxy);
  }
  for (int i=0;i<WS_MAXFREELISTS;i++)
  {
    while (StringFreeList[i])
    {
      void **proxy=StringFreeList[i];
      StringFreeList[i]=(void **)(*proxy);
      free(proxy);
    }
    StringFreeBytes[i]=0;
  }
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
}



size_t WStringMemory::GetStatistics(size_t *details)
{
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  size_t sum=0;
  for (int i=0;i<WS_MAXFREELISTS;i++)
  {
    sum+=StringFreeBytes[i];
    if (details) details[i]=StringFreeBytes[i];
  }
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
  return sum;
}

struct ShareDBItem
{
  WStringProxy *_str;
  ShareDBItem(WStringProxy *str=NULL):_str(str) {}
  int Compare(const ShareDBItem& other) const 
	{
	if (_str==NULL) return other._str!=NULL?1:0;
	if (other._str==NULL) return -1;
	return wcscmp(_str->_redirect,other._str->_redirect);
	}

  bool operator==(const ShareDBItem& other) const {return Compare(other)==0;}
  bool operator>=(const ShareDBItem& other) const {return Compare(other)>=0;}
  bool operator<=(const ShareDBItem& other) const {return Compare(other)<=0;}
  bool operator!=(const ShareDBItem& other) const {return Compare(other)!=0;}
  bool operator>(const ShareDBItem& other) const {return Compare(other)>0;}
  bool operator<(const ShareDBItem& other) const {return Compare(other)<0;}
};

static BTree<ShareDBItem> *GDB=NULL;

WStringProxy *WStringMemory::ShareString(WStringProxy *proxy)
{
  if (proxy->_operation!=WStringProxy::OpMemBlck || proxy->_blockData2==0 || proxy->_blockData2==2) return proxy;

#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif

  if (GDB==NULL) GDB=new BTree<ShareDBItem>;

  proxy->_blockData2=2;	  //block is subject of sharing
  proxy->_redirect=proxy->GetStringFromMemBlock(); //setup pointer to string
  ShareDBItem *found=GDB->Find(ShareDBItem(proxy)); 
  if (found) {proxy->_blockData2=1;proxy=found->_str;}
  else GDB->Add(ShareDBItem(proxy));
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
  return proxy;
}

void WStringMemory::UnshareString(WStringProxy *proxy)
{
  if (proxy->_operation!=WStringProxy::OpMemBlck || proxy->_blockData2!=2) return;
  if (GDB==NULL) return;
#ifdef WSTRING_MT
  EnterCriticalSection(&GLocker);
#endif
  GDB->Remove(ShareDBItem(proxy));  
  proxy->_blockData2=1;
#ifdef WSTRING_MT
  LeaveCriticalSection(&GLocker);
#endif
}