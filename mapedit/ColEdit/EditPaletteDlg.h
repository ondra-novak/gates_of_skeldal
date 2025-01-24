#if !defined(AFX_EDITPALETTEDLG_H__8ED55BD3_E477_4C1A_80E7_D79CDA43A55E__INCLUDED_)
#define AFX_EDITPALETTEDLG_H__8ED55BD3_E477_4C1A_80E7_D79CDA43A55E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditPaletteDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditPaletteDlg dialog

#include "PaletteEditor.h"
#include "..\Podlahar\ImageView.h"

class CEditPaletteDlg : public CDialog
{
// Construction
  char _paletteUndo[768];
  char _paletteScrl[768];
  float _curh,_curv,_curs;
public:
	CEditPaletteDlg(CWnd* pParent = NULL);   // standard constructor
	char *_palette;
	CImageView *_imgView;
	

// Dialog Data
	//{{AFX_DATA(CEditPaletteDlg)
	enum { IDD = IDD_EDITPALETTE };
	CSliderCtrl	wSitost;
	CSliderCtrl	wJas;
	CSliderCtrl	wBarva;
	CPaletteEditor	wPaleta;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditPaletteDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CalculateAvgHSV();

	// Generated message map functions
	//{{AFX_MSG(CEditPaletteDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaleta();
	virtual void OnCancel();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnUndo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITPALETTEDLG_H__8ED55BD3_E477_4C1A_80E7_D79CDA43A55E__INCLUDED_)
