#if !defined(AFX_DLGDIALOGY_H__E47269D5_6849_42F5_AF6F_32EEEE32104D__INCLUDED_)
#define AFX_DLGDIALOGY_H__E47269D5_6849_42F5_AF6F_32EEEE32104D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgDialogy.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DlgDialogy dialog

#define MAXDIALOGY 240

class CMainFrame;

class DlgDialogy : public CDialog
{
// Construction
public:
	void LoadDialogyDlg();
	DlgDialogy(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(DlgDialogy)
	enum { IDD = IDD_DIALOGY };
	CListCtrl	wList;
	//}}AFX_DATA

	Pathname _dlgpath;
	CMainFrame *_mainFrame;
	int _sortItem;
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DlgDialogy)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DlgDialogy)
	virtual BOOL OnInitDialog();
	afx_msg void OnRescan();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnColumnclickDlglist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int GetDialogID(const char *name);
	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
	  DlgDialogy *self=(DlgDialogy *)lParamSort;
	  return self->CompareItems(lParam1,lParam2);
	}

	int CompareItems(int item1, int item2);


public:
  afx_msg void OnNMDblclkDlglist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnLvnEndlabeleditDlglist(NMHDR *pNMHDR, LRESULT *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGDIALOGY_H__E47269D5_6849_42F5_AF6F_32EEEE32104D__INCLUDED_)
