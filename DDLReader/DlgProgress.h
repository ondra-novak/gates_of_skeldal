#pragma once
#include "t:\h\atlmfc\include\afxcmn.h"
#include "t:\h\atlmfc\include\afxwin.h"


// DlgProgress dialog

class DlgProgress : public CDialog
{
	DECLARE_DYNAMIC(DlgProgress)

public:
	DlgProgress(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgProgress();

// Dialog Data
	enum { IDD = IDD_EXPORTING };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  CProgressCtrl wProgress;
  CStatic wDesc;
  afx_msg void OnBnClickedButton1();
  bool stop;
};
