// WStringProxy.h: interface for the WStringProxy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WSTRINGPROXY_H__863ADE82_7789_4E54_BE7D_B8F0740FD81B__INCLUDED_)
#define AFX_WSTRINGPROXY_H__863ADE82_7789_4E54_BE7D_B8F0740FD81B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WStringMemory.h"
#include "IWStringEffect.h"
#include <WCHAR.H>
#include <string.h>
#include <assert.h>
#include <malloc.h>

/*
List of proxy types

Memory_Proxy      _operation=OpMemBlck
                  _blockData==0 and _blockData2==0
                  or 
                  _blockData!=0 and _blockData2!=0
				  (_redirect - debug pointer)
				  (_blockData - 1)

Proxy_Const       _operation=OpMemBlck
                  _redirect - pointer to string
                  _blockData2=0

Proxy_Shared      _operation=OpMemBlck
                  _redirect - pointer to string
                  _blockData2=2;

Proxy_Immediate   _operation=OpMemBlck
                  _blockData>0xFFFF
                  _blockData2>0xFFFF
                  both parameters are used by immediate memory manager

Proxy_Concat      _operation=OpConcat
                  _baseString and _secondString is valid

Proxy_Link        _operation=OpSubstr
                  _baseString is valid
                  _stringSize==_baseString->_stringSize
                  _offset==0;
                  NOTE: Can be used without rendering

Proxy_RightSub    _operation=OpSubstr
                  _baseString is valid
                  _offset<=_baseString->_stringSize
                  _stringSize==_baseString->_stringSize-_offset
                  NOTE: Can be used without rendering

Proxy_SubString   _operation=OpSubstr
                  _baseString is valid
                  _offset<=_baseString->_stringSize
                  _stringSize<=_baseString->_stringSize-_offset

Proxy_Effect      _operation=OpEffect
                  _baseString is valid
                  _stringSize=_baseString->_stringSize
                  _effect defining effect code < 0xFFFF

Proxy_UserEffect  _operation=OpEffect
                  _baseString is valid
                  _stringSize=_baseString->_stringSize
                  _effect>0xFFFF
                  _userEffect defining pointer to effect interface

*/
class WStringProxy  
{
public:
  enum Operation 
  {
	OpConcat=0,	//proxy contains two links to concat it
	OpSubstr=1,	//proxy contains link to another proxy and specified substring
	OpMemBlck=-2,	//proxy contains informations about following string
	OpEffect=-1,	//proxy describing some efect with string
  };

  enum Effect
  {
	EfLower,	//effect lower case
	EfUpper,	//effect upper case
	EfReverse,	//effect reverse string
  };
public:

  unsigned long _refCount;  //reference count
  unsigned long _stringSize:30;  //string size in characters (maximum size 1073741823 characters ~ 2147483646 bytes)
  Operation _operation:2;		//operation with string or proxy type

  union
  {
	WStringProxy *_baseString;	//pointer to next proxy referenced by this proxy
	unsigned long _blockData;	//user defined block data for OpMemBlock proxy type
	const wchar_t *_redirect;   //used for OpMemBlock, when _blockData2 is zero.
  };

  union
  {
	WStringProxy *_secondString;  //pointer to second string for OpConcat
	unsigned long _offset;		  //offset of substring for OpSubstr
	Effect _effect;				  //effect selector for OpEffect
	IWStringEffect *_userEffect;  //user effect defined by IWStringEffect interface (valid when _effect is invalid)
	unsigned long _blockData2;	//user defined block data for OpMemBlock proxy type - exception: if this value is zero, member _redirect is valid
	
  };


  void RenderStringToBuffer(wchar_t *renderPtr);
  void RenderStringToBufferSubstr(wchar_t *renderPtr, size_t offset, size_t size);
  WStringProxy *TransitivniUzaver();
  void RenderSimpleEffect(wchar_t *renderPtr);

public:
  WStringProxy():_refCount(0),_operation(OpSubstr),_stringSize(0),_baseString(0),_secondString(0) {} //inicializes empty string proxy

  WStringProxy(WStringProxy *other):
	  _refCount(0),
	  _stringSize(other->GetLength()),
	  _baseString(other),
	  _operation(OpSubstr),
	  _offset(0) 
	  {_baseString->AddRef();} 

  WStringProxy(WStringProxy *other, unsigned long offset, unsigned long size):
	  _refCount(0),
	  _stringSize(size),
	  _baseString(other),
	  _operation(OpSubstr),
	  _offset(offset) 
	  {_baseString->AddRef();} 
	  
  WStringProxy(WStringProxy *a, WStringProxy *b):
	  _refCount(0),
	  _stringSize(a->GetLength()+b->GetLength()),
	  _baseString(a),
	  _operation(OpConcat),
	  _secondString(b)
	  {_baseString->AddRef();_secondString->AddRef();} 

  WStringProxy(WStringProxy *a, Effect effect):
	  _refCount(0),
	  _stringSize(a->GetLength()),
	  _baseString(a),
	  _operation(OpEffect),
	  _effect(effect)
	  {_baseString->AddRef();} 

  WStringProxy(WStringProxy *a, IWStringEffect *userEffect):
	  _refCount(0),
	  _stringSize(a->GetLength()+userEffect->GetEffectExtraSize(a->GetLength())),
	  _baseString(a),
	  _operation(OpEffect),
	  _userEffect(userEffect)
	  {_baseString->AddRef();} 

  WStringProxy(unsigned long size, unsigned long user1, unsigned long user2):
	  _refCount(0),
	  _stringSize(size),
	  _operation(OpMemBlck),
	  _blockData(user1),
	  _blockData2(user2)
	  {} 

  WStringProxy(const wchar_t *imText):
	  _refCount(0),
	  _stringSize(wcslen(imText)),
	  _redirect(imText),
	  _blockData2(0),
	  _operation(OpMemBlck)
	  {} 

  WStringProxy(const WStringProxy &other)
  {
	memcpy(this,&other,sizeof(*this));
	if (_operation!=OpMemBlck) _baseString->AddRef();
	if (_operation==OpConcat) _secondString->AddRef();
  }

  WStringProxy& operator=(const WStringProxy &other)
  {	
	WStringProxy::~WStringProxy();			//call destructor to destruct current proxy
	memcpy(this,&other,sizeof(*this));		//construct new proxy from template
	if (_operation!=OpMemBlck) _baseString->AddRef();
	if (_operation==OpConcat) _secondString->AddRef();
  }

  WStringProxy *RenderString();

  ~WStringProxy()
  {
  	if (_operation!=OpMemBlck) _baseString->Release();
	if (_operation==OpConcat) _secondString->Release();
  }


  unsigned long GetLength() {return _stringSize;}
  void AddRef() {if (this) WStringMemory::AddRefProxy(this);}
  void Release() {if (this) if (WStringMemory::ReleaseRefProxy(this)) WStringMemory::FreeProxy(this);}

  const wchar_t *GetString()
  {
	if (_operation==OpMemBlck) return (const wchar_t *)(this+1);
	WStringProxy *p=RenderString();
	(*this)=*p;
	return (const wchar_t *)(p+1);
  }

  const wchar_t *GetStringFromMemBlock()
  {
	assert(_operation==OpMemBlck);
	if (_blockData2==0 && _redirect!=NULL) return _redirect;
	else return (const wchar_t *)(this+1);
  }

  bool IsShared() {return _refCount>1;}

  void RecalcLength()
  {
	const wchar_t *str=GetStringFromMemBlock();
  	_stringSize=wcslen(str);	
  }

};

#endif // !defined(AFX_WSTRINGPROXY_H__863ADE82_7789_4E54_BE7D_B8F0740FD81B__INCLUDED_)
