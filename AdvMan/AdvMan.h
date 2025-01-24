// AdvMan.h : main header file for the ADVMAN application
//

#if !defined(AFX_ADVMAN_H__A43CD3B3_508C_4DDB_B89C_50FE1291B8B9__INCLUDED_)
#define AFX_ADVMAN_H__A43CD3B3_508C_4DDB_B89C_50FE1291B8B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAdvManApp:
// See AdvMan.cpp for the implementation of this class
//

class CAdvManApp : public CWinApp
{
public:
	CAdvManApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdvManApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	//{{AFX_MSG(CAdvManApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  virtual BOOL PreTranslateMessage(MSG *pMsg);
};


/////////////////////////////////////////////////////////////////////////////

extern CAdvManApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADVMAN_H__A43CD3B3_508C_4DDB_B89C_50FE1291B8B9__INCLUDED_)
