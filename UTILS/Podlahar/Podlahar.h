// Podlahar.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CPodlaharApp:
// See Podlahar.cpp for the implementation of this class
//

class CPodlaharApp : public CWinApp
{
public:
	CPodlaharApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CPodlaharApp theApp;