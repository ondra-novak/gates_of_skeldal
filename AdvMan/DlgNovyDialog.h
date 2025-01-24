#if !defined(AFX_DLGNOVYDIALOG_H__00136ADF_E2C6_4081_83AB_D5110EE43612__INCLUDED_)
#define AFX_DLGNOVYDIALOG_H__00136ADF_E2C6_4081_83AB_D5110EE43612__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgNovyDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DlgNovyDialog dialog

class DlgDialogy;

class DlgNovyDialog : public CDialog
{
// Construction
public:
	void DialogRules();
	DlgNovyDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(DlgNovyDialog)
	enum { IDD = IDD_NOVYDIALOG };
	CComboBox	wCislo;
	CComboBox	wJmeno;
	int		vCislo;
	CString	vJmeno;
	CString	vPopis;
	//}}AFX_DATA

	CListCtrl *sourceLst;
	Pathname dlgSource;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DlgNovyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DlgNovyDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeJmeno();
	afx_msg void OnSelendokJmeno();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGNOVYDIALOG_H__00136ADF_E2C6_4081_83AB_D5110EE43612__INCLUDED_)
