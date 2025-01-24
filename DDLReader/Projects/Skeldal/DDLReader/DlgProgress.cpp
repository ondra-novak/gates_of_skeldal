// DlgProgress.cpp : implementation file
//

#include "stdafx.h"
#include "DDLReader.h"
#include "DlgProgress.h"
#include ".\dlgprogress.h"


// DlgProgress dialog

IMPLEMENT_DYNAMIC(DlgProgress, CDialog)
DlgProgress::DlgProgress(CWnd* pParent /*=NULL*/)
	: CDialog(DlgProgress::IDD, pParent)
{
  stop=false;
}

DlgProgress::~DlgProgress()
{
}

void DlgProgress::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_PROGRESS, wProgress);
  DDX_Control(pDX, IDC_NAME, wDesc);
}


BEGIN_MESSAGE_MAP(DlgProgress, CDialog)
  ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// DlgProgress message handlers

void DlgProgress::OnBnClickedButton1()
{
  stop=true;
}
