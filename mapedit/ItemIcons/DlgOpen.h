#pragma once
#include "afxwin.h"


// DlgOpen dialog

class DlgOpen : public CDialog
{
	DECLARE_DYNAMIC(DlgOpen)
    CString _umisteni;
    CString _selected;
    char *_iconlib;
    CIconViewerButton icons[18];

public:
	DlgOpen(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgOpen();

// Dialog Data
	enum { IDD = IDD_OPENDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


	DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnInitDialog();
  void ReadFolder(void);
  CListBox wList;
  afx_msg void OnBnClickedBrowse();
    CString GetSelectedFile();    
protected:
  virtual void OnOK();
public:
  afx_msg void OnLbnDblclkList();
  afx_msg void OnDestroy();
  afx_msg void OnLbnSelchangeList();
};
