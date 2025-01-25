// WStringProxy.cpp: implementation of the WStringProxy class.
//
//////////////////////////////////////////////////////////////////////


#include "WStringProxy.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/** Funkce vytvo�� transitivn� uz�v�r.
to znamen�, �e zru�� v�echny z�et�zen� redirecty.
*/
WStringProxy *WStringProxy::TransitivniUzaver()
{
  if (_operation==OpSubstr && _offset==0 && _stringSize==_baseString->GetLength())
  {
	WStringProxy *next=_baseString->TransitivniUzaver();
	if (next==NULL) return this;
	next->AddRef();
    _baseString->Release();
    _baseString=next;
	return next;
  }
  return NULL;
}

/** Main render procedure
It creates allocates buffer proxy and renders tree into it
Then turns self into redirect proxy a redirect self into newly created string
Rerurns pointer to newly created proxy
*/
WStringProxy *WStringProxy::RenderString()
{
  if (_operation==OpMemBlck) return this;
    
  WStringProxy *pp=TransitivniUzaver();
  if (pp!=NULL) return pp->_baseString->RenderString();

  WStringProxy *newProxy=WStringMemory::AllocString(NULL,_stringSize);
  wchar_t *renderPtr=const_cast<wchar_t *>(newProxy->GetStringFromMemBlock());

  RenderStringToBuffer(renderPtr);

  newProxy->AddRef();
  this->~WStringProxy();
  _operation=OpSubstr;
  _offset=0;
  _baseString=newProxy;

  return newProxy;
}

/** Function renders simple effect into buffer
Function needs buffer with nul terminated string
*/
void WStringProxy::RenderSimpleEffect(wchar_t *renderPtr)
{
  assert(_operation==OpEffect);
  switch (_effect)
  {
  case EfLower: wcslwr(renderPtr);break;
  case EfUpper: wcsupr(renderPtr);break;
  case EfReverse: wcsrev(renderPtr);break;
  }

}

/** Light phase of rendering. Used to render string that is not
created from substring. This part is optimized for concat and user effects
When substring must be rendered, function call RenderStringToBufferSubstr to 
render substring.
*/
void WStringProxy::RenderStringToBuffer(wchar_t *renderPtr)
{
  WStringMemory::LockProxy(this);
  switch (_operation)
  {
  case OpConcat:
	_baseString->RenderStringToBuffer(renderPtr);
	_secondString->RenderStringToBuffer(renderPtr+_baseString->GetLength());
	break;
  case OpSubstr:
	{
      _baseString->RenderStringToBufferSubstr(renderPtr,_offset,GetLength());
	}
	break;
  case OpMemBlck:
	{
	  const wchar_t *str=GetStringFromMemBlock();
	  wcsncpy(renderPtr,str,_stringSize);
	}
	break;
  case OpEffect:	
	{
	  uint32_t offset=0;
	  renderPtr[_stringSize]=0;		  //we can append zero, because right side of string is not yet rendered  
	  //if this is end of string, one extra character for zero is also allocated.
	  //efect functions can rely on it.
	  if (_blockData2>=256) offset=_userEffect->PreRenderString(renderPtr,_baseString->GetLength()); //call prerender to prepare begin of buffer
	  _baseString->RenderStringToBuffer(renderPtr+offset);		//render string to rest of buffer
	  if (_blockData2>=256) _userEffect->RenderString(renderPtr,_baseString->GetLength()); //apply effect to buffer
	  else RenderSimpleEffect(renderPtr);
	}
	break;
  };
  WStringMemory::UnlockProxy(this);
}

/**
Deep phase of rendering. Function can render substrings, or render substring of two 
concated strings. Function can also perform partial effect on string. Function cannot
handle partial user effect, so this effects are converted to things strings.
*/
void WStringProxy::RenderStringToBufferSubstr(wchar_t *renderPtr, size_t offset, size_t size)
{
  WStringMemory::LockProxy(this);
  switch (_operation)
  {
  case OpConcat:
    {
      //process substring of concat
      //count characters in buffer
      size_t rendered;     
      //when string starts in left string
      if (_baseString->GetLength()>offset)
      {
        //calculate total characters that may be rendered
        rendered=_baseString->GetLength()-offset;
        //but limit it to request size
        if (rendered>size) rendered=size;
        //render substring into buffer
        _baseString->RenderStringToBufferSubstr(renderPtr,offset,rendered);
      }
      else
        //no character has been rendered
        rendered=0;

      //there is still characters remained to render. We will take it from second string
      if (size-rendered>0)
      {
        if (offset>_baseString->GetLength()) offset-=_baseString->GetLength();
        else offset=0;
        _secondString->RenderStringToBufferSubstr(renderPtr+rendered,offset,size-rendered);
      }
    }
    break;
  case OpSubstr:
    { //rendering substrings is very easy
      offset+=_offset;  //add offset of substring
      assert(offset+size<=GetLength()); //check length
      //render substring
      this->RenderStringToBufferSubstr(renderPtr,offset,size);
    }
    break;
  case OpMemBlck:
    {
      //rendering from memory, final stop in recursion
      //Get pointer string 
      const wchar_t *str=GetStringFromMemBlock();
      //copy substring from string into render buffer
      wcsncpy(renderPtr,str+offset,size);
    }
    break;
  case OpEffect:	
      if (_blockData2>=256) //interface cannot handle partial rendering
      {
        RenderString();   //convert proxy to simple redirect and render it
        RenderStringToBufferSubstr(renderPtr,offset,size); //now we are able to cut out part
      }
      else
      {
        //all standard effects maps string 1:1
        //first get content of substring into buffer
        _baseString->RenderStringToBufferSubstr(renderPtr,offset,size);                
        //we can append zero, because right side of string is not yet rendered  
        renderPtr[size]=0;
        //process effect on target
        RenderSimpleEffect(renderPtr);
      }
    break;
  };
  WStringMemory::UnlockProxy(this);
}