// DDLReader.h : main header file for the DDLREADER application
//

#if !defined(AFX_DDLREADER_H__9FE8F7F8_112D_4735_A4BA_5141A991D609__INCLUDED_)
#define AFX_DDLREADER_H__9FE8F7F8_112D_4735_A4BA_5141A991D609__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDDLReaderApp:
// See DDLReader.cpp for the implementation of this class
//

class CDDLReaderApp : public CWinApp
{
public:
	CDDLReaderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDDLReaderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDDLReaderApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DDLREADER_H__9FE8F7F8_112D_4735_A4BA_5141A991D609__INCLUDED_)
