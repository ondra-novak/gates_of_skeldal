// WStringMemory.h: interface for the WStringMemory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WSTRINGMEMORY_H__79693029_4788_4099_9A97_92AF310A7AD5__INCLUDED_)
#define AFX_WSTRINGMEMORY_H__79693029_4788_4099_9A97_92AF310A7AD5__INCLUDED_

#include <WCHAR.H>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class WStringProxy;

class WStringMemory
{
public:

	static WStringProxy * AllocString(const wchar_t *text, size_t size);
	static WStringProxy * AllocProxy(const WStringProxy &templateProxy);
	static void FreeProxy(WStringProxy *proxy);
	static void LockProxy(WStringProxy *proxy);
	static void UnlockProxy(WStringProxy *proxy);

	//Releases extra memory leaved for future fast allocations
	static void FreeExtra();

	//Support addref for proxy - Single- or Multi- thread support
	static void AddRefProxy(WStringProxy *proxy);

	//Support addref for proxy - Single- or Multi- thread support
	//returns true, when counter reached zero
	static bool ReleaseRefProxy(WStringProxy *proxy);

    ///Gets statistics about memory manager
    /** 
    @param details optional pointer to 32 size_t items. Array will be filled
      with sizes of each string fastalloc group.
    @return function returns total bytes allocated for string fast allocation.
      (This number should be below 256KB)
    */
    static size_t GetStatistics(size_t *details=NULL);

    ///Function allows sharing the strings
	/** This function is called by WString::Share function
	Function mark proxy shared. If string is same as shared,
	it returns pointer to proxy with proxy of string that contains
	the same text.
	Read description of WString::Share for more informations
	*/
	static WStringProxy *ShareString(WStringProxy *proxy);

	///Function disables sharing of the string
	static void UnshareString(WStringProxy *proxy);

};



#endif // !defined(AFX_WSTRINGMEMORY_H__79693029_4788_4099_9A97_92AF310A7AD5__INCLUDED_)
