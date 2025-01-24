// PodlaháøDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "ColorButton.h"
#include "ImageView.h"


// CPodlahDlg dialog
class CPodlahDlg : public CDialog
{
// Construction
public:
	CPodlahDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PODLAH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedBrowse();
  int type;
  CString vHlTextura;
  CString vVdTextura;
  CColorButton wColorButt;
  CString vTarget;
  CButton wBrowse1;
  CButton wBrowse2;
  CButton wBrowse3;
  CButton wSavePic;
  CImageView wPreview;
  afx_msg void OnBnClickedColor();
  afx_msg void OnBnClickedPreview();
  BOOL vGenMlhu;
  afx_msg void OnBnClickedSavepic();
  afx_msg void OnBnClickedBrowseSave();
  int vType;
};


extern "C"
{
    char *PodlahaStrop(int r, int g, int b, const char *filename1, const char *filename2, int genMlhu);
    char *GetPodlahaPalette();
    int save_pcx(char *filename,int x1,int y1,int x2,int y2);
    char CheckPCX(const char *pcx);

}