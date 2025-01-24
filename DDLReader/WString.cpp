// WString.cpp: implementation of the WString class.
//
//////////////////////////////////////////////////////////////////////

#include "WString.h"
#include <windows.h>
#include <stdio.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WString::WString(const char *sbstring, unsigned int codePage)
{
  if (sbstring==NULL || *sbstring==0) 
  {
	_ref=NULL;
  }
  else
  {
  size_t reqBuff=MultiByteToWideChar(codePage,0,sbstring,-1,NULL,0);
  
  _ref=WStringMemory::AllocString(NULL,reqBuff);
  _ref->AddRef();
  wchar_t *str=const_cast<wchar_t *>(_ref->GetStringFromMemBlock());
  
  MultiByteToWideChar(codePage,0,sbstring,-1,str,reqBuff);  
  }
}

int WString::FormatV(wchar_t *format, va_list lst)
{
  size_t curSize=4096;
  int written;
  do
  {
	_ref=WStringMemory::AllocString(NULL,curSize);
	wchar_t *str=const_cast<wchar_t *>(_ref->GetStringFromMemBlock());
	written=_vsnwprintf(str,curSize,format,lst);
	if (written<0)
	{
	  curSize*=2;
	  WStringMemory::FreeProxy(_ref);
	}
  }
  while (written<-1);
  _ref->RecalcLength();
  _ref->AddRef();
  return written;
}

int WString::Format(wchar_t *format, ...)
{
  va_list valst;
  va_start(valst,format);
  return FormatV(format,valst);
}


int WString::ScanStringV(const wchar_t *format, va_list lst) const
{
  unsigned long *ptr=(unsigned long *)lst;
  return swscanf(GetString(),format,ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7],ptr[8],ptr[9],
									ptr[10],ptr[11],ptr[12],ptr[13],ptr[14],ptr[15],ptr[16],ptr[17],ptr[18],ptr[19]);

}

int WString::ScanString(const wchar_t *format, ...) const
{
  va_list valst;
  va_start(valst,format);
  return ScanStringV(format,valst);
}

bool WString::ReplaceOnce(const WString &findWhat,const WString &replaceWith)
{
  int pos=Find(findWhat);
  if (pos==-1) return false;
  (*this)=Left(pos)+replaceWith+Right(pos+replaceWith.GetLength());
  return true;
}

bool WString::ReplaceAll(const WString &findWhat,const WString &replaceWith)
{
  WString process=*this;
  WString result;
  int pos;
  bool processed=false;
  while ((pos=process.Find(findWhat))!=-1)
  {
	result=result+process.Left(pos)+replaceWith;
	process=process.Right(pos+findWhat.GetLength());
	processed=true;
  }
  *this=result+process;
  return processed;
}

WString WString::TrimLeft() const
{
  return TrimLeft(L" \r\n\t");
}

WString WString::TrimRight() const
{
  return TrimRight(L" \r\n\t");
}

WString WString::TrimLeft(wchar_t *trimChars) const
{
  const wchar_t *proStr=GetString();
  int p=0;
  while (proStr[p] && wcschr(trimChars,proStr[p]!=NULL)) p++;
  return Right(p);
}

WString WString::TrimRight(wchar_t *trimChars) const
{
  const wchar_t *proStr=GetString();
  int p=GetLength()-1;
  while (p>=0 && wcschr(trimChars,proStr[p]!=NULL)) p--;
  return Left(p+1);
}

WString WString::Upper() const
{
  return WString(WStringMemory::AllocProxy(WStringProxy(_ref,WStringProxy::EfUpper)));
}

WString WString::Lower() const
{
  return WString(WStringMemory::AllocProxy(WStringProxy(_ref,WStringProxy::EfLower)));
}

WString WString::Reverse() const
{
  return WString(WStringMemory::AllocProxy(WStringProxy(_ref,WStringProxy::EfReverse)));
}

WString WString::Effect(IWStringEffect *effect) const
{
  return WString(WStringMemory::AllocProxy(WStringProxy(_ref,effect)));
}

void WString::SetUTF7(const char *utf7)
{
  *this=WString(utf7,CP_UTF7);
}

void WString::SetUTF8(const char *utf8)
{
  *this=WString(utf8,CP_UTF8);
}

const char *WString::AsSBString(unsigned int codePage, WString &holder)
{
  const wchar_t *str=GetString();
  if (str[0]==0) return "";
  size_t reqsize=WideCharToMultiByte(codePage,0,str,-1,NULL,0,NULL,NULL);
  WStringProxy *holderProxy=WStringMemory::AllocString(NULL,reqsize/2); //reqsize/2+(2 bytes) 
  char *mbstr=reinterpret_cast<char *>(const_cast<wchar_t *>(holderProxy->GetStringFromMemBlock()));
  WideCharToMultiByte(codePage,0,str,-1,mbstr,reqsize,NULL,NULL);
  holder=WString(holderProxy);
  return mbstr;
}

const char *WString::AsUTF7(WString &holder)
{
  return AsSBString(CP_UTF7,holder);
}

const char *WString::AsUTF8(WString &holder)
{
  return AsSBString(CP_UTF8,holder);
}

void WString::ReadFromStream(size_t (*streamReader)(wchar_t *buffer, size_t bufferLen, void *context),void *context)
{
  _ref->Release();
  _ref=NULL;
  wchar_t buff[256];
  size_t rd;
  while ((rd=streamReader(buff,sizeof(buff)/sizeof(wchar_t),context))!=0)
  {
	*this=*this+WString(buff,rd);
  }
}

const WString WStringConst(const wchar_t *text)
{
  return WString(WStringMemory::AllocProxy(WStringProxy(text)));
}

