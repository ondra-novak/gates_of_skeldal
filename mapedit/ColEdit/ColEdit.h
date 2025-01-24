// ColEdit.h : main header file for the COLEDIT application
//

#if !defined(AFX_COLEDIT_H__4A70A254_5D3A_4703_9034_4EE5A90DFE73__INCLUDED_)
#define AFX_COLEDIT_H__4A70A254_5D3A_4703_9034_4EE5A90DFE73__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CColEditApp:
// See ColEdit.cpp for the implementation of this class
//

class CColEditApp : public CWinApp
{
public:
	CColEditApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColEditApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CColEditApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLEDIT_H__4A70A254_5D3A_4703_9034_4EE5A90DFE73__INCLUDED_)
