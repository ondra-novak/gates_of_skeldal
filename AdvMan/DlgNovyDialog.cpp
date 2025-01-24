// DlgNovyDialog.cpp : implementation file
//

#include "stdafx.h"
#include "AdvMan.h"
#include "DlgNovyDialog.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DlgNovyDialog dialog


DlgNovyDialog::DlgNovyDialog(CWnd* pParent /*=NULL*/)
	: CDialog(DlgNovyDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(DlgNovyDialog)
	vCislo = 0;
	vJmeno = _T("");
	vPopis = _T("");
	//}}AFX_DATA_INIT
}


void DlgNovyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DlgNovyDialog)
	DDX_Control(pDX, IDC_CISLO, wCislo);
	DDX_Control(pDX, IDC_JMENO, wJmeno);
	DDX_Text(pDX, IDC_CISLO, vCislo);
	DDV_MinMaxInt(pDX, vCislo, 1, 254);
	DDX_CBString(pDX, IDC_JMENO, vJmeno);
	DDV_MaxChars(pDX, vJmeno, 8);
	DDX_Text(pDX, IDC_POPIS, vPopis);
	DDV_MaxChars(pDX, vPopis, 50);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DlgNovyDialog, CDialog)
	//{{AFX_MSG_MAP(DlgNovyDialog)
	ON_CBN_EDITCHANGE(IDC_JMENO, OnEditchangeJmeno)
	ON_CBN_SELENDOK(IDC_JMENO, OnSelendokJmeno)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DlgNovyDialog message handlers

BOOL DlgNovyDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	bool idlist[256];
	memset(idlist,0,sizeof(idlist));

	dlgSource.SetFilename("*.dlg");
	CFileFind fnd;
	BOOL next;
	if (fnd.FindFile(dlgSource)) do
	{
	  next=fnd.FindNextFile();
	  CString name=fnd.GetFileTitle();
	  name.MakeLower();
	  wJmeno.AddString(name);
	}while (next);

	int i,cnt;
	for (i=0,cnt=sourceLst->GetItemCount();i<cnt;i++)
	{
	  CString name=sourceLst->GetItemText(i,1);
	  dlgSource.SetFilename(name);
	  name=dlgSource.GetTitle();
	  name.MakeLower();
	  int p=wJmeno.FindStringExact(-1,name);
	  if (p!=-1) wJmeno.DeleteString(p);

	  name=sourceLst->GetItemText(i,2);
	  p=atoi(name);
	  if (p>=0 && p<256) idlist[p]=true;
	}
	bool x=true;
	for (i=1;i<254;i++) if (!idlist[i])
	{
	  char buff[50];
	  sprintf(buff,"%3d",i);
	  wCislo.AddString(buff);
	  if (x)
	  {
    	wCislo.SetWindowText(buff);
		x=false;
	  }
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void DlgNovyDialog::DialogRules()
{
  BOOL ok=wJmeno.GetWindowTextLength() && wCislo.GetWindowTextLength();
  GetDlgItem(IDOK)->EnableWindow(ok);
  CString name;
  wJmeno.GetWindowText(name);
  dlgSource.SetFiletitle(name);
  wCislo.EnableWindow(access(dlgSource,0)==-1);
} 

void DlgNovyDialog::OnEditchangeJmeno() 
{
  DialogRules();	
}

void DlgNovyDialog::OnSelendokJmeno() 
{
  int p=wJmeno.GetCurSel();
  CString z;
  wJmeno.GetLBText(p,z);
  wJmeno.SetWindowText(z);
  DialogRules();		
}
