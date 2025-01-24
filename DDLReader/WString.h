// WString.h: interface for the WString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WSTRING_H__481164FB_3BB7_4824_B0E3_8B371F0AAF3A__INCLUDED_)
#define AFX_WSTRING_H__481164FB_3BB7_4824_B0E3_8B371F0AAF3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WStringProxy.h"
#include <stdlib.h>

class WString  
{
  mutable WStringProxy *_ref;
  
  
  
public:
  ///constructs empty string
  WString():_ref(0) {}

  ///constructs string. 
  explicit WString(const wchar_t *string):_ref(string==NULL?NULL:WStringMemory::AllocString(string,0)) {_ref->AddRef();};

  ///constructs string from wide-char array with specified length
  WString(const wchar_t *string, size_t sz):_ref(WStringMemory::AllocString(string,sz)) {_ref->AddRef();};

  ///copy constructor
  /**constructor doesn't copy the string, only reference. 
  String is shared until it is changed
  */
  WString(const WString &other):_ref(other._ref) {_ref->AddRef();}  
 
  ///constructs string from multi-byte string. codePage specified code page of string
  WString(const char *sbstring, unsigned int codePage);
  
  ///constructs string from string-proxy. Used internally with WStringMemory::AllocXXX functions
  WString(WStringProxy *proxy):_ref(proxy) {_ref->AddRef();}

  ///assignment operator
  /** assignment operator doesn't copy the string only reference.
    String is shared until it is changed */
  WString &operator=(const WString &other) 
    {other._ref->AddRef();_ref->Release();_ref=other._ref;return *this;}
  
  ///Destructor - releases reference
  ~WString() {_ref->Release();}
  
  ///function converts instance of WString to wchar_t pointer
  /**
  Function will perform RenderString, if it is needed.  
  */
  const wchar_t *GetString() const
  {
	if (_ref==NULL) return L"";
	WStringProxy *str=_ref->RenderString();
	if (_ref!=str) 
	{
	  str->AddRef();_ref->Release();_ref=str;
	}
	return _ref->GetStringFromMemBlock();
  }
  
  ///operator can convert WString to wchar_t pointer anytime 
  operator const wchar_t *() const {return GetString();}
  
  ///function returns count of characters in string
  /** function doesn't need to render string, so it can be called anytime 
  without loosing the benefit of "pseudo-strings" boosting */
  size_t GetLength() const
  {
	if (_ref) return _ref->GetLength();
	else return 0;
  }
  
  ///function creates string as sum of this and another string
  /** function creates "pseudo-string" that represents sum of two string.
  pseudo-string is rendered to "real-string" automatically, when it is needed */
  WString operator+(const WString other) const
  {
	if (_ref==NULL) return other;
	if (other._ref==NULL) return *this;
	return WString(WStringMemory::AllocProxy(WStringProxy(_ref,other._ref)));
  }

  WString &operator+=(const WString other) 
  {
    *this=*this+other;
    return *this;
  }

  ///function creates string as substring of another string
  /** function creates "pseudo-string" that represents substring of anothers string.
  Pseudo-string is rendered to "real-string" automatically, when it is needed */
  WString Mid(size_t begin, size_t len) const
  {
	if (begin==0) return Left(len);
	if (_ref==NULL || begin>GetLength()) return WString();
	if (begin+len>GetLength()) return Right(begin);
	return WString(WStringMemory::AllocProxy(WStringProxy(_ref,begin,len)));
  }
  
  WString Left(size_t len) const
  {
	if (_ref==NULL) return WString();
	if (len>=GetLength()) return *this;
	return WString(WStringMemory::AllocProxy(WStringProxy(_ref,0,len)));
  }
  
  WString Right(size_t begin) const
  {	  
	if (_ref==NULL || begin>GetLength()) return WString();
	if (begin==0) return *this;
	return WString(WStringMemory::AllocProxy(WStringProxy(_ref,begin,GetLength()-begin)));
  }
  
  WString RightR(size_t count) const
  {
	if (_ref==NULL || count==0) return WString();
	if (count>=GetLength()) return *this;
	return WString(WStringMemory::AllocProxy(WStringProxy(_ref,GetLength()-count,count)));
  }
  
  WString Delete(size_t begin, size_t count) const
  {
	if (_ref==NULL) return WString();
	if (begin==0) return Right(count);
	if (begin+count>=GetLength()) return Left(begin);
	  return WString(WStringMemory::AllocProxy(WStringProxy(Left(begin)._ref,Right(begin+count)._ref)));
  }
  
  WString Insert(const WString &what, size_t index) const
  {
	if (_ref==NULL) return what;
	if (index==0) return what+*this;
	if (index>=GetLength()) return *this+what;
	return Left(index)+what+Right(index);
  }

  /// function allows to access any char in string
  wchar_t operator[](int index) const
  {
	assert(index<=(int)GetLength() && index>=0);
	return GetString()[index];
  }
 
  /// functions allows lock string for accesing it directly.
  /** Function returns pointer to buffer which holds string.
  Buffer size is equal to string length plus terminating zero.
  Application can modify content of buffer.
  If string is shared with another WString object, LockBuffer
  creates copy of string
  Do not forger to call UnlockBuffer, when you finish modifiing the content
  */
  wchar_t *LockBuffer()
  {
	if (_ref==NULL) return NULL;
	GetString();
	WStringMemory::LockProxy(_ref);
	if (_ref->IsShared())
	{
	  WStringMemory::UnlockProxy(_ref);
	  _ref->Release();
	  _ref=WStringMemory::AllocString(_ref->GetStringFromMemBlock(),0);
	  _ref->AddRef();
	  WStringMemory::LockProxy(_ref);
	}
	return const_cast<wchar_t *>(_ref->GetStringFromMemBlock());
  }
  
  /// Function creates buffer for string and returns its address
  /** Application can use buffer in functions, that cannot work with
  WString objects. Size of buffer is specified by sz value, and it is in characters.
  Application must call UnlockBuffer after writes all data into buffer.

  NOTE: Prevoius content of string is lost.
  NOTE: sz specifies buffer size without terminating zero character
  */
  wchar_t *CreateBuffer(size_t sz)
  {
	_ref->Release();
	_ref=WStringMemory::AllocString(NULL,sz);
	_ref->AddRef();
	WStringMemory::LockProxy(_ref);
	return const_cast<wchar_t *>(_ref->GetStringFromMemBlock());
  }
  
  /// Function creates buffer, and copies string into it.
  /** Parameter sz specifies size of new buffer in characters
  When sz is smaller then size of string, string is truncated
  When sz is larger then size of string, string is copied unchanged,
  but extra characters can be appended.

  NOTE: sz specifies buffer size without terminating zero character
  */
  wchar_t *CreateBufferCopy(size_t sz)
  {
	WString save=*this;
	wchar_t *wbuff=CreateBuffer(sz);
	int minsz=__min(sz,save.GetLength());
	wcsncpy(wbuff,save.GetString(),minsz);
	return wbuff;
  }
  
  /// Function unlocks internal buffer
  /** parameter sz specifies final lenght of string in buffer. Default
  value -1 forces function calc size by own
  */
  void UnlockBuffer(int sz=-1)
  {
	if (_ref==NULL) return;	
	wchar_t *wbuff=const_cast<wchar_t *>(_ref->GetStringFromMemBlock());
	if (sz<0) sz=wcslen(wbuff);
	else wbuff[sz]=0;
	if (sz!=(signed)_ref->GetLength())
	{
	  _ref->RecalcLength();
	  WStringMemory::UnlockProxy(_ref);
	}
  }
  
  class WStringAtCharHelper
  {
	WString &_str;
	size_t _index;
  public:
	WStringAtCharHelper(WString &str,size_t index):_str(str),_index(index) {}
	WString &operator=(wchar_t z)
	{
	  wchar_t *buff=_str.LockBuffer();
	  assert(buff && _index<_str.GetLength());
	  buff[_index]=z;
	  _str.UnlockBuffer();
	  return _str;
	}
	operator wchar_t() 
	{
  	  assert(_index<_str.GetLength());	  
	  return ((const wchar_t *)_str)[_index];
	}
  };
  
  /// Function will return helper object, which can access single character in string
  /**
  Using array operator is slower than accessing characters by LockBuffer function
  */
  WStringAtCharHelper operator[](int index)
  {
	return WStringAtCharHelper(*this,index);
  }

  int Find(wchar_t z,size_t from=0) const
  {
	if (from>=GetLength()) return -1;
	const wchar_t *res=wcschr(GetString()+from,z);
	if (res) return res-GetString();
	else return -1;
  }

  int FindLast(wchar_t z) const
  {
	const wchar_t *res=wcsrchr(GetString(),z);
	if (res) return res-GetString();
	else return -1;
  }

  int Find(const wchar_t *z,size_t from=0) const
  {
	if (z==NULL) return -1;
	if (from>=GetLength()) return -1;
	const wchar_t *res=wcsstr(GetString()+from,z);
	if (res) return res-GetString();
	else return -1;
  }
  
  int FormatV(wchar_t *format, va_list lst);
  
  int Format(wchar_t *format, ...);

  ///Scans string for format
  /** function is limited, item count is limited up to 20 items */
  int ScanStringV(const wchar_t *format, va_list lst) const;

  ///Scans string for format
  /** function is limited, item count is limited up to 20 items */
  int ScanString(const wchar_t *format, ...) const;

  bool ReplaceOnce(const WString &findWhat,const WString &replaceWith);

  bool ReplaceAll(const WString &findWhat,const WString &replaceWith);

  WString TrimLeft() const;
  
  WString TrimRight() const;

  WString TrimLeft(wchar_t *trimChars) const;
  
  WString TrimRight(wchar_t *trimChars) const;

  ///Function splits string into two
  /** Left part of string is returned.
	  Right part of string is leaved in object
  @param splitPos split position. 
  @return if splitPos==0, function returns empty string. if splitPos>=length, function
	  moves string from object to result*/
  WString Split(size_t splitPos)
  {
	WString result=Left(splitPos);
	*this=Right(splitPos);
	return result;
  }

  bool IsEmpty() const {return _ref==NULL;}

  void Empty() const {_ref->Release();_ref=NULL;}

  int Compare(const wchar_t *other) const
  {
	return wcscmp(*this,other);
  }

  bool operator>(const WString &other) const {return Compare(other)>0;}
  bool operator<(const WString &other) const {return Compare(other)<0;}
  bool operator>=(const WString &other) const {return Compare(other)>=0;}
  bool operator<=(const WString &other) const {return Compare(other)<=0;}
  bool operator!=(const WString &other) const {return Compare(other)<=0;}
  bool operator==(const WString &other) const {return Compare(other)<=0;}

  WString Upper() const;
  WString Lower() const;
  WString Reverse() const;

  ///Applies user effect on string
  /** pointer should be static allocated object, or must be valid during lifetime of resulting string */
  WString Effect(IWStringEffect *effect) const;
  
  void SetUTF7(const char *utf7);
  void SetUTF8(const char *utf8);

  ///Function converts string to SingleByte string
  /** 
  @param codePage Platform depends code page of result
  @param holder Object that holds result. Result pointer is valid during lifetime of holder. Once holder
  released, pointer is invalidated. You can get resulting pointer anytime from holder reintepreting 
  wchar_t to char
  @result pointer to string
  */
  const char *AsSBString(unsigned int codePage, WString &holder);

  ///Returns multibyte string in UTF7 codepage
  /**See AsSBString description*/
  const char *AsUTF7(WString &holder);

  ///Returns multibyte string in UTF7 codepage
  /**See AsSBString description*/
  const char *AsUTF8(WString &holder);


  ///Function reads string from stream
  /** 
  Function calls streamReader repeatly until function returns zero. In each call, streamReader returns
  number of characters, that has been readed. 
  @param sreamReader pointer to function which provides reading from a stream
  @param context user defined context pointer (most in cases, pointer to stream is passed)
  @param buffer space allocated for data readed from stream
  @param bufferLen maximum count of characters can be written to buffer
  @return streamReader returns number of characters, that has been written to buffer. Function must return zero
	to stop reading
  */
  void ReadFromStream(size_t (*streamReader)(wchar_t *buffer, size_t bufferLen, void *context),void *context);

  WString operator() (int from) const {return from<0?RightR(-from):Right(from);}
  WString operator() (int from, int to) const 
  {
  if (from>=0)  
	return to<0?Mid(from,-to):Mid(from,to-from);
  else
	return to<0?Mid(GetLength()+from,-to):Mid(GetLength()+from,GetLength()-from-to);  
  }

  ///Enables global string sharing.
  /** Function Share allows share strings that contains same text. It can reduce memory
  usage. Function is useful when there is many objects that contains string with same text.

  Basically, two strings is shared, if one string is created as copy of another string. Strings 
  is sharing they content, until one of them is modified. 

  Calling Share function, string is registered for global sharing. If string with same content is already 
  registered, then content of current string is released, and string is shared. 

  To successfully share two strings, both strings must call Share() function. Sharing is provided until one 
  of strings is modified. After modifications are done, Share function of modified string must be called again.

  Remember: Global sharing can reduce amount of memory, if there are multiple strings that containging 
  the same content. But sharing of each unique string takes small piece of memory to store sharing 
  informations.

  To share string automatically, use WSharedString class
  WSC strings cannot be shared!
  */
  void Share() const
  {
	WStringProxy *curref=_ref;
	GetString();
	_ref=WStringMemory::ShareString(curref);
	_ref->AddRef();
	curref->Release();
  }

  ///Disables global string sharing
  /** Function unregisters string from global sharing database. If string is shared, function doesn't break 
  it. But all strings that currently sharing content will not be shared with newly created strings. 
  */
  void Unshare()
  {
	WStringMemory::UnshareString(_ref);
  }
};

///Function will create constant WString
/** function will not copy content of string. Only creates reference to string
  Using WStringConst is faster for strings in code
@exmple result=a+WStringConst(L"text")+b;
*/
const WString WStringConst(const wchar_t *text);

///Macro to easy convert incode string  to WString
/**
function also marks text wide.
@example WString text=WSC("Hello world");
*/
#define WSC(x) WStringConst(L##x)
///Macro to easy convert any wchar_t buffer to constant WString
/**
IMPORTANT NOTE!: Buffer must exists until string is rendered. 
If you are using WSCB in function, don't forget call GetString before function exits
If you are excepting additional string operations outside function, using standard
WString conversion can be more effecient.
*/
#define WSCB(x) WStringConst(x)



class WSharedString: public WString
{
public:
  WSharedString() {}

  ///constructs string. 
  explicit WSharedString(const wchar_t *string):WString(string) {Share();}

  ///constructs string from wide-char array with specified length
  WSharedString(const wchar_t *string, size_t sz):WString(string,sz) {Share();};

  ///copy constructor
  /**constructor doesn't copy the string, only reference. 
  String is shared until it is changed
  */
  WSharedString(const WSharedString &other):WString(other) {}
  WSharedString(const WString &other):WString(other) {Share();}
 
  ///constructs string from multi-byte string. codePage specified code page of string
  WSharedString(const char *sbstring, unsigned int codePage):WString(sbstring,codePage) {Share();}
  
  ///assignment operator
  /** assignment operator doesn't copy the string only reference.
    String is shared until it is changed */
  WSharedString &operator=(const WString &other) 
	{WString::operator =(other);Share();return *this;}

  const wchar_t *GetString() const
  {
	const wchar_t *res=WString::GetString();
	Share();
	return res;
  }
  operator const wchar_t *() const {return GetString();}
  wchar_t operator[](int index) const
  {
	assert(index<=(int)GetLength() && index>=0);
	return GetString()[index];
  }

  void UnlockBuffer(int sz=-1)
  {
	WString::UnlockBuffer(sz);
	Share();	
  }
  
};


#endif // !defined(AFX_WSTRING_H__481164FB_3BB7_4824_B0E3_8B371F0AAF3A__INCLUDED_)
