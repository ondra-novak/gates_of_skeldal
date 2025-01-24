// DlgNoveDobr.cpp : implementation file
//

#include "stdafx.h"
#include "AdvMan.h"
#include "DlgNoveDobr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DlgNoveDobr dialog


DlgNoveDobr::DlgNoveDobr(CWnd* pParent /*=NULL*/)
	: CDialog(DlgNoveDobr::IDD, pParent)
{
	//{{AFX_DATA_INIT(DlgNoveDobr)
	vJmeno = _T("");
	vKouzla = TRUE;
	vMapy = FALSE;
	vDialogy = TRUE;
	vDefinice = FALSE;
	vOrganizace = 0;
	vStartMap = _T("start.map");
	vMaxPostav = 3;
	vMinPostav = 3;
	//}}AFX_DATA_INIT
}


void DlgNoveDobr::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DlgNoveDobr)
	DDX_Text(pDX, IDC_JMENO, vJmeno);
	DDX_Check(pDX, IDC_KOUZLA, vKouzla);
	DDX_Check(pDX, IDC_MAPY, vMapy);
	DDX_Check(pDX, IDC_DIALOGY, vDialogy);
	DDX_Check(pDX, IDC_DEFINICE, vDefinice);
	DDX_Radio(pDX, IDC_ORGANIZACE, vOrganizace);
	DDX_Text(pDX, IDC_STARTMAP, vStartMap);
  DDX_Check(pDX, IDC_ORIGDLGS, vDialogyDlg);
	DDV_MaxChars(pDX, vStartMap, 12);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_MINPOSTAV, vMinPostav);
	DDV_MinMaxUInt(pDX, vMinPostav, 1, 6);
	DDX_Text(pDX, IDC_MAXPOSTAV, vMaxPostav);
	DDV_MinMaxUInt(pDX, vMaxPostav, vMinPostav, 6);
}


BEGIN_MESSAGE_MAP(DlgNoveDobr, CDialog)
	//{{AFX_MSG_MAP(DlgNoveDobr)
	ON_EN_CHANGE(IDC_JMENO, OnChangeJmeno)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DlgNoveDobr message handlers

void DlgNoveDobr::DialogRules()
{
  GetDlgItem(IDOK)->EnableWindow(GetDlgItem(IDC_JMENO)->GetWindowTextLength());
}

BOOL DlgNoveDobr::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	DialogRules();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void DlgNoveDobr::OnChangeJmeno() 
{
  DialogRules();	
}
