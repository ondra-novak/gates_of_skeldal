// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__0420AD88_2B58_4379_BD0D_069A821C039D__INCLUDED_)
#define AFX_MAINFRM_H__0420AD88_2B58_4379_BD0D_069A821C039D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ChildView.h"


class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:
	TSTR_LIST _adv;
	CString _baseMapPath;
	CString _advPath;
	CString _advName;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	void StartApp(const char *appname, CString cmdline, bool wait, const char *folder=0);
	void OpenEditor(const char *name);
	void SaveAll();
	static LRESULT BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void SetTitle(const _TCHAR *name);
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
  CToolBar    m_wndEditoryBar;
  CToolBar    m_wndPrekladaceBar;
	CEdit    m_wndView;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnFileNovdobrodrust();
	afx_msg void OnFileNatidobrodrustv();
	afx_msg void OnEditoryKouzlatab();
	afx_msg void OnEditoryPostavytab();
	afx_msg void OnEditoryItemsscr();
	afx_msg void OnEditoryItemspic();
	afx_msg void OnEditoryWeaponsscr();
	afx_msg void OnDestroy();
	afx_msg void OnPekladaePelokouzla();
	afx_msg void OnPekladaePelodialogy();
	afx_msg void OnPekladaePelopostavytab();
	afx_msg void OnNstrojeMapedit();
	afx_msg void OnNstrojeTestujdobrodrustv();
	afx_msg void OnNstrojeTvrcepodlah();
	afx_msg void OnNstrojeTvrcepaletpronestvry();
	afx_msg void OnNstrojeTvrceikonpropedmty();
	afx_msg void OnEditorySouboradv();
	afx_msg void OnFileZnovunast();
	afx_msg void OnUpdateFileZnovunast(CCmdUI* pCmdUI);
	afx_msg void OnEditoryDialogy();
  afx_msg void OnUpdateEditory(CCmdUI *pCmdUI);
//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__0420AD88_2B58_4379_BD0D_069A821C039D__INCLUDED_)
