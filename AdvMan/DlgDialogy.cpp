// DlgDialogy.cpp : implementation file
//

#include "stdafx.h"
#include "AdvMan.h"
#include "DlgDialogy.h"
#include "../cztable.h"
#include "MainFrm.h"
#include "DlgNovyDialog.h"
#include <io.h>
#include ".\dlgdialogy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DlgDialogy dialog


DlgDialogy::DlgDialogy(CWnd* pParent /*=NULL*/)
	: CDialog(DlgDialogy::IDD, pParent)
{
	//{{AFX_DATA_INIT(DlgDialogy)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void DlgDialogy::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DlgDialogy)
	DDX_Control(pDX, IDC_DLGLIST, wList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DlgDialogy, CDialog)
	//{{AFX_MSG_MAP(DlgDialogy)
	ON_BN_CLICKED(IDC_RESCAN, OnRescan)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_DLGLIST, OnColumnclickDlglist)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
  ON_NOTIFY(NM_DBLCLK, IDC_DLGLIST, OnNMDblclkDlglist)
	//}}AFX_MSG_MAP
  ON_NOTIFY(LVN_ENDLABELEDIT, IDC_DLGLIST, OnLvnEndlabeleditDlglist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DlgDialogy message handlers

BOOL DlgDialogy::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	wList.InsertColumn(0,CString(MAKEINTRESOURCE(IDS_DLGDESC)),LVCFMT_LEFT,200,0);
	wList.InsertColumn(1,CString(MAKEINTRESOURCE(IDS_DLGNAME)),LVCFMT_LEFT,100,1);
	wList.InsertColumn(2,CString(MAKEINTRESOURCE(IDS_DLGID)),LVCFMT_CENTER,30,2);
	ListView_SetExtendedListViewStyleEx(wList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	OnRescan();		
	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int DlgDialogy::GetDialogID(const char *name)
{
  Pathname dialogPath=_dlgpath;
  dialogPath.SetFilename(name);
  FILE *f=fopen_icase(dialogPath,"r");
  if (f==0) return -1;  
  int id;
  char dialog[40];
  if (fscanf(f," %40[^\r\n\t (] ( %d )",dialog,&id)==2)
  {
	if (stricmp(dialog,"DIALOG")==0)
	{
	  fclose(f);
	  return id;
	}
  }
  fclose(f);
  return -1;
}

void DlgDialogy::LoadDialogyDlg()
{ 
  FILE *f=fopen_icase(_dlgpath,"r");
  if (f==0) return;

  int p=0;

  while (!feof(f))
  {
	char name[80];
	char desc[256];
	name[0]=0;
	desc[0]=0;
	fscanf(f,"%80s",name);
	fscanf(f," %250[^\r\n]",desc);
	kamenik2windows(name,strlen(name),name);
	kamenik2windows(desc,strlen(desc),desc);
	int row;

	if (name[0]!=0 || desc[0]!=0)
	{
	  row=wList.InsertItem(p,desc,0);
	  wList.SetItemText(row,1,name);
	  int dlgId=GetDialogID(name);
	  wList.SetItemText(row,2,itoa(dlgId,desc,10));
    p++;
	}
  }
  fclose(f);
}

void DlgDialogy::OnRescan() 
{
  wList.DeleteAllItems();
  LoadDialogyDlg();
}

void DlgDialogy::OnDelete() 
{
  int w;
  while ((w=wList.GetNextItem(-1,LVNI_SELECTED))!=-1)
  {
	wList.DeleteItem(w);
  }
}

void DlgDialogy::OnEdit() 
{
  CString name;
  int w=-1;
  while ((w=wList.GetNextItem(w,LVNI_SELECTED))!=-1)
  {
	name=wList.GetItemText(w,1);
	Pathname editName=_dlgpath;
	editName.SetFilename(name);
	_mainFrame->OpenEditor(editName);
  }
}

void DlgDialogy::OnOK() 
{
  FILE *f=fopen_icase(_dlgpath,"w");
  if (f==0) 
  {
    AfxMessageBox(IDS_UNABLETOSAVEDIALOGYDLG,MB_ICONEXCLAMATION);
    return;
  }
	
  for (int i=0,cnt=wList.GetItemCount();i<cnt;i++)
  {
    CString name=wList.GetItemText(i,1);
    CString desc=wList.GetItemText(i,0);
    windows2kamenik(name,name.GetLength(),name.LockBuffer());
    windows2kamenik(desc,desc.GetLength(),desc.LockBuffer());
    name.UnlockBuffer();
    desc.UnlockBuffer();
    if (desc.Trim().GetLength()==0)
      desc.LoadString(IDS_NODESC);
    if (name.Trim().GetLength()) fprintf(f,"%s %s\n",name.GetString(),desc.GetString());
  }
	fclose(f);
	CDialog::OnOK();
}

void DlgDialogy::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void DlgDialogy::OnColumnclickDlglist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	for (int i=0,cnt=wList.GetItemCount();i<cnt;i++)
	{
	  wList.SetItemData(i,i);
	}

	_sortItem=pNMListView->iSubItem;
	wList.SortItems(CompareFunc,(DWORD)this);

	*pResult = 0;
}


int DlgDialogy::CompareItems(int item1, int item2)
{
  switch (_sortItem)
  {
  case 0:
	{
	  CString a=wList.GetItemText(item1,0);
	  CString b=wList.GetItemText(item2,0);
	  return stricmp(a,b);
	}
  case 1:
	{
	  CString a=wList.GetItemText(item1,1);
	  CString b=wList.GetItemText(item2,1);
	  return stricmp(a,b);
	}
  case 2:
	{
	  char buff[20];
	  int a,b;
	  wList.GetItemText(item1,2,buff,20);a=atoi(buff);
	  wList.GetItemText(item2,2,buff,20);b=atoi(buff);
	  return (a>b)-(a<b);
	}
  }
  return 0;
}

void DlgDialogy::OnAdd() 
{
  DlgNovyDialog nw;
  nw.dlgSource=this->_dlgpath;
  nw.sourceLst=&wList;
  if (nw.DoModal()==IDOK)
  {
	int p=wList.InsertItem(wList.GetItemCount(),nw.vPopis);
	Pathname dlgName=_dlgpath;
	dlgName.SetFiletitle(nw.vJmeno);
	dlgName.SetExtension(".dlg");
	wList.SetItemText(p,1,dlgName.GetFilename());
	if (_check_file_exists(dlgName)!=0)
	{
	  FILE *f=fopen_icase(dlgName,"w");
	  fprintf(f,"DIALOG(%d)\n",nw.vCislo);
	  fprintf(f,"{\nPICTURE (\"SCREEN\")\nDESC (\"Popis\")\nSTANDARD (1)\n}\n\nSENTENCE (1,0)\n{\n\n}");
	  fclose(f);
	}
	int num=GetDialogID(dlgName.GetFilename());
	char buff[20];
	wList.SetItemText(p,2,itoa(num,buff,10));	
  }
}


void DlgDialogy::OnNMDblclkDlglist(NMHDR *pNMHDR, LRESULT *pResult)
{
  OnEdit();
  *pResult = 0;
}

void DlgDialogy::OnLvnEndlabeleditDlglist(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
  // TODO: Add your control notification handler code here
  *pResult = 1;
}
