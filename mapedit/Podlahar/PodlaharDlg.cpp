// PodlaháøDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Podlahar.h"
#include "PodlaharDlg.h"
#include ".\podlahardlg.h"
#include "..\..\DDLReader\WString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CPodlahDlg dialog



CPodlahDlg::CPodlahDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPodlahDlg::IDD, pParent)    
    , vHlTextura(_T(""))
    , vVdTextura(_T(""))
    , vTarget(_T(""))
    , vGenMlhu(FALSE)
    , vType(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPodlahDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_HLTEXTURA, vHlTextura);
  DDX_Text(pDX, IDC_VDTEXTURA, vVdTextura);
  DDX_Control(pDX, IDC_COLOR, wColorButt);
  DDX_Text(pDX, IDC_CILOVYSOUBOR, vTarget);
  DDX_Control(pDX, IDC_BROWSE1, wBrowse1);
  DDX_Control(pDX, IDC_BROWSE2, wBrowse2);
  DDX_Control(pDX, IDC_BROWSE3, wBrowse3);
  DDX_Control(pDX, IDC_SAVEPIC, wSavePic);
  DDX_Control(pDX, IDC_IMAGE, wPreview);
  DDX_Check(pDX, IDC_GENMLHU, vGenMlhu);
  DDX_Radio(pDX, IDC_TYP, vType);
}

BEGIN_MESSAGE_MAP(CPodlahDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BROWSE1, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BROWSE2, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BROWSE3, OnBnClickedBrowseSave)
    ON_BN_CLICKED(IDC_COLOR, OnBnClickedColor)
    ON_BN_CLICKED(IDC_PREVIEW, OnBnClickedPreview)
    ON_BN_CLICKED(IDC_SAVEPIC, OnBnClickedSavepic)
END_MESSAGE_MAP()


// CPodlahDlg message handlers

BOOL CPodlahDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPodlahDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPodlahDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPodlahDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPodlahDlg::OnBnClickedBrowse()
{
  CString pszFilePath;  
  GetFocus()->GetNextWindow(GW_HWNDPREV)->GetWindowText(pszFilePath);
  CString filter;filter.LoadString(IDS_PCXFILTER);  
  CString title;title.LoadString(IDS_BROWSEPCX);
  CFileDialog fdlg(TRUE,_T("PCX"),pszFilePath,OFN_PATHMUSTEXIST|OFN_HIDEREADONLY,filter);
  if (fdlg.DoModal()==IDOK)
  {
	pszFilePath=fdlg.GetPathName();
#ifdef _UNICODE
    char *name=(char *)alloca(pszFilePath.GetLength()+1);
    WideCharToMultiByte(CP_ACP,0,pszFilePath,pszFilePath.GetLength()+1,name,pszFilePath.GetLength()+1,NULL,NULL);
#else
    const char *name=pszFilePath;
#endif
	if (CheckPCX(name)==0)
	{
	  GetFocus()->GetNextWindow(GW_HWNDPREV)->SetWindowText(fdlg.GetPathName());
	}
	else
	  AfxMessageBox(IDS_ERRORPCX);
  }  
}

void CPodlahDlg::OnBnClickedBrowseSave()
{
  CString pszFilePath;  
  GetFocus()->GetNextWindow(GW_HWNDPREV)->GetWindowText(pszFilePath);
  CString filter;filter.LoadString(IDS_PCXFILTER);  
  CString title;title.LoadString(IDS_BROWSEPCX);
  CFileDialog fdlg(FALSE,_T("PCX"),pszFilePath,OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,filter);
  if (fdlg.DoModal())
  {
	  GetFocus()->GetNextWindow(GW_HWNDPREV)->SetWindowText(fdlg.GetPathName());
  }  
}

void CPodlahDlg::OnBnClickedColor()
{
  CColorDialog dlg(wColorButt._color,CC_FULLOPEN|CC_ANYCOLOR|CC_RGBINIT);
  if (dlg.DoModal()==IDOK)
  {
    wColorButt.SetColor(dlg.GetColor());
  }
}

void CPodlahDlg::OnBnClickedPreview()
{
  UpdateData(TRUE);
#ifdef _UNICODE
  WString hlTex(vHlTextura.GetString());
  WString vlTex(vVdTextura.GetString());   
  COLORREF color=wColorButt._color;
  char *result=PodlahaStrop(GetRValue(color), GetGValue(color), GetBValue(color), 
    hlTex.AsUTF8(hlTex),vlTex.AsUTF8(vlTex),vGenMlhu);
#else
  const char *hlTex=vHlTextura;
  const char *vlTex=vVdTextura;
  COLORREF color=wColorButt._color;
  char *result=PodlahaStrop(GetRValue(color), GetGValue(color), GetBValue(color), 
    hlTex,vlTex,vGenMlhu);
#endif
  wPreview.UpdateBuffer(result+512+6);
  wPreview.SetPalette(GetPodlahaPalette());

}

void CPodlahDlg::OnBnClickedSavepic()
{
  UpdateData(TRUE);
  if (vTarget.GetLength())
  {
    OnBnClickedPreview();
#ifdef _UNICODE
    WString name(vTarget.GetString());
    char *cname=const_cast<char *>(name.AsUTF8(name));
#else
    char *cname=const_cast<char *>((LPCTSTR)vTarget);
#endif
    if (vType)    
      save_pcx(cname,0,0,639,89);
    else
      save_pcx(cname,0,360-198,639,360);     
    ShellExecute(*this,NULL,vTarget,0,NULL,SW_SHOWNORMAL);
  } 
}
