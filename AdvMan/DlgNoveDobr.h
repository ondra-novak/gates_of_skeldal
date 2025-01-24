#if !defined(AFX_DLGNOVEDOBR_H__410472E5_5BCF_4CE8_8FE3_10693C0F05F7__INCLUDED_)
#define AFX_DLGNOVEDOBR_H__410472E5_5BCF_4CE8_8FE3_10693C0F05F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgNoveDobr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DlgNoveDobr dialog

class DlgNoveDobr : public CDialog
{
// Construction
public:
	void DialogRules();
	DlgNoveDobr(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(DlgNoveDobr)
	enum { IDD = IDD_NOVEDOBR };
	CString	vJmeno;
	BOOL	vKouzla;
	BOOL	vMapy;
	BOOL	vDialogy;
	BOOL	vDefinice;
  BOOL  vDialogyDlg;
	int		vOrganizace;
	CString	vStartMap;
	UINT	vMaxPostav;
	UINT	vMinPostav;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DlgNoveDobr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DlgNoveDobr)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeJmeno();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGNOVEDOBR_H__410472E5_5BCF_4CE8_8FE3_10693C0F05F7__INCLUDED_)
