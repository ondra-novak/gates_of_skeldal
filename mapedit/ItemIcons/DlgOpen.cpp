// DlgOpen.cpp : implementation file
//

#include "stdafx.h"
#include "ItemIcons.h"
#include "iconviewerbutton.h"
#include "DlgOpen.h"


// DlgOpen dialog

IMPLEMENT_DYNAMIC(DlgOpen, CDialog)
DlgOpen::DlgOpen(CWnd* pParent /*=NULL*/)
	: CDialog(DlgOpen::IDD, pParent)
{
}

DlgOpen::~DlgOpen()
{
}

void DlgOpen::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  for (int i=0;i<18;i++)
    DDX_Control(pDX, IDC_ICON1+i, icons[i]);
  DDX_Control(pDX, IDC_LIST, wList);
}


BEGIN_MESSAGE_MAP(DlgOpen, CDialog)
  ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
  ON_LBN_DBLCLK(IDC_LIST, OnLbnDblclkList)
  ON_WM_DESTROY()
  ON_LBN_SELCHANGE(IDC_LIST, OnLbnSelchangeList)
END_MESSAGE_MAP()


// DlgOpen message handlers

BOOL DlgOpen::OnInitDialog()
{
  CDialog::OnInitDialog();
  ReadFolder();
  _iconlib=new char[ICONLIBSIZE];
  ZeroMemory(_iconlib,ICONLIBSIZE);
  for (int i=0;i<18;i++) icons[i]._data=GETICONADDR(i);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

void DlgOpen::ReadFolder(void)
{
  wList.ResetContent();
  CFileFind fnd;  
  BOOL ok=fnd.FindFile(_umisteni+_T("ikony*.lib"));
  while (ok)
  {
    ok=fnd.FindNextFile();
    wList.AddString(fnd.GetFileName());
  }
  CString createnew;
  createnew.LoadString(IDS_CREATENEW);
  int index=wList.AddString(createnew);
  wList.SetItemData(index,1);
}

static int WINAPI PosBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
  {
  const _TCHAR *curpath=(const _TCHAR *)lpData;  
  if (uMsg == BFFM_INITIALIZED)
    {
      if (curpath && curpath[0])
      {
        ::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)((LPCSTR)curpath));
        ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)((LPCSTR)curpath));
      }
    }
    else if (uMsg == BFFM_SELCHANGED)
      {
      _TCHAR buff[_MAX_PATH];
      if (SHGetPathFromIDList((LPITEMIDLIST)lParam,buff))
        {        
        ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)buff);
        }
      }
  return 0;
  };

static bool PathBrowser(HWND hWnd, _TCHAR *path /* MAX_PATH size */)
{
  BROWSEINFO brw;
  memset(&brw,0,sizeof(brw));
  brw.hwndOwner=hWnd;
  brw.pidlRoot=NULL;
  brw.pszDisplayName=path;
  brw.lParam=(LPARAM)path;
  brw.ulFlags= BIF_RETURNONLYFSDIRS |BIF_STATUSTEXT|BIF_USENEWUI ; 	
  brw.lpfn = (BFFCALLBACK)(PosBrowseCallbackProc);
  LPITEMIDLIST il=SHBrowseForFolder( &brw );
  if (il==NULL) return false;
  SHGetPathFromIDList(il,path);
  IMalloc *shmalloc;
  SHGetMalloc(&shmalloc);
  shmalloc->Free(il);
  if (path[0]==0) return false;
  return true;
}


void DlgOpen::OnBnClickedBrowse()
{
  _TCHAR buffer[MAX_PATH+1];
  _tcsncpy(buffer,_umisteni,MAX_PATH);
  if (PathBrowser(*this,buffer))
    _umisteni=buffer;
  if (_umisteni.GetLength() && _umisteni[_umisteni.GetLength()-1]!='\\')
    _umisteni=_umisteni+'\\';
  ReadFolder();
}

void DlgOpen::OnOK()
{
  int sel=wList.GetCurSel();
  if (sel<0) return;
  CString name;
  if (wList.GetItemData(sel)==1)
  {
    int i=-1;
    do
    {
      i++;
      name.Format(_T("IKONY%02d.LIB"),i);
      _selected=_umisteni+name;
    }
    while (_tcheck_file_exists( _selected)==0);
  }
  else
  {
    wList.GetText(sel,name);
    _selected=_umisteni+name;
  }
  CDialog::OnOK();
}


CString DlgOpen::GetSelectedFile()
{
  return _selected;
}

void DlgOpen::OnLbnDblclkList()
{
OnOK();
}

void DlgOpen::OnDestroy()
{
  delete [] _iconlib;
}

void DlgOpen::OnLbnSelchangeList()
{
  ZeroMemory(_iconlib,ICONLIBSIZE);
  int sel=wList.GetCurSel();
  if (sel>=0) 
  {
    CString name;
    if (wList.GetItemData(sel)!=1)
    {
      wList.GetText(sel,name);
      _selected=_umisteni+name;
      HANDLE h=CreateFile(_selected,GENERIC_READ,0,0,OPEN_EXISTING,0,0);
      if (h!=INVALID_HANDLE_VALUE)
      {
        DWORD rd;
        ReadFile(h,_iconlib,ICONLIBSIZE,&rd,0);
        CloseHandle(h);
      } 
    }
  }
  for (int i=0;i<18;i++) icons[i].Invalidate();
}
