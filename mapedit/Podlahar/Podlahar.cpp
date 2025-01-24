// Podlaháø.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Podlahar.h"
#include "PodlaharDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPodlahApp

BEGIN_MESSAGE_MAP(CPodlahApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPodlahApp construction

CPodlahApp::CPodlahApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CPodlahApp object

CPodlahApp theApp;


// CPodlahApp initialization

BOOL CPodlahApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CPodlahDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


extern "C"
{
    showview() {abort();}
  screen_buffer_size() {abort();}
  DxDoneWalk() {abort();}
  DxZoomWalk() {abort();}
  DxPrepareWalk() {abort();}
  get_timer_value() {abort();}
  DxDoneTurn() {abort();}
  DxTurn() {abort();}
  DxPrepareTurn() {abort();}
  GetBuffer2nd() {abort();}
  GetScreenAdr() {abort();}
  put_picture() {abort();}
  put_textured_bar_() {abort();}
  RestoreScreen() {abort();}
  outtext() {abort();}
  set_aligned_position() {abort();}
  trans_bar() {abort();}
  text_width() {abort();}
  RedirectScreenBufferSecond() {abort();}
  void *getmem(long size) {return malloc(size);}
}