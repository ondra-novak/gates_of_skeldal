// DDLReaderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DDLReader.h"
#include "DDLReaderDlg.h"
#include ".\ddlreaderdlg.h"
#include "WPathname.h"
#include "DlgProgress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDDLReaderDlg dialog

CDDLReaderDlg::CDDLReaderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDDLReaderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDDLReaderDlg)
	vFolder = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CDDLReaderDlg::~CDDLReaderDlg()
{
  if (!_lastTemp.IsNull()) DeleteFile(_lastTemp);
}

void CDDLReaderDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CDDLReaderDlg)
  DDX_Control(pDX, IDC_FOLDER, wFolder);
  DDX_Control(pDX, IDC_FILELIST, wFileList);
  DDX_Control(pDX, IDC_BROWSE, wBrowse);
  DDX_Control(pDX, IDC_EXPORT, wExport);
  DDX_Text(pDX, IDC_FOLDER, vFolder);
  DDX_Control(pDX, IDC_POPISEK, wPopisek);
  //}}AFX_DATA_MAP
  DDX_Control(pDX, IDC_DDLFILE, wDDLFile);
  DDX_Control(pDX, IDC_DDLBROWSE, wDDLBrowse);
}

BEGIN_MESSAGE_MAP(CDDLReaderDlg, CDialog)
	//{{AFX_MSG_MAP(CDDLReaderDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_EN_KILLFOCUS(IDC_DDLFILE, OnEnKillfocusDdlfile)
    ON_BN_CLICKED(IDC_DDLBROWSE, OnBnClickedDdlbrowse)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_FILELIST, OnLvnColumnclickFilelist)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_EXPORT, OnBnClickedExport)
    ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDDLReaderDlg message handlers

BOOL CDDLReaderDlg::OnInitDialog()
{
    CRect rc;
    GetClientRect(&rc);
    _dlgSize=rc.Size();
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

    CString header;
    header.LoadString(IDC_HEADGROUP);
    wFileList.InsertColumn(0,header,LVCFMT_CENTER,100,0);
    header.LoadString(IDC_HEADFNAME);
    wFileList.InsertColumn(1,header,LVCFMT_LEFT,140,1);
    header.LoadString(IDC_HEADSIZE);
    wFileList.InsertColumn(2,header,LVCFMT_RIGHT,80,2);
    header.LoadString(IDC_HEADOFFSET);
    wFileList.InsertColumn(3,header,LVCFMT_RIGHT,80,3);
    ListView_SetExtendedListViewStyleEx(wFileList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDDLReaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDDLReaderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDDLReaderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void MoveWindowRel(HDWP &hdwp, CWnd &wnd, int xr,int yr, int xs, int ys)
{
  CRect rc;
  wnd.GetWindowRect(&rc);
  wnd.GetParent()->ScreenToClient(&rc);
  rc.left+=xr;
  rc.top+=yr;
  rc.bottom+=yr+ys;
  rc.right+=xr+xs;
  hdwp=DeferWindowPos(hdwp,wnd,NULL,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,SWP_NOZORDER);
}

void CDDLReaderDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialog::OnSize(nType, cx, cy);
  if (nType!=SIZE_MINIMIZED)
  {
    if (wFileList.GetSafeHwnd()!=0) 
    {
      HDWP dwp=BeginDeferWindowPos(10);
      int difx=cx-_dlgSize.cx;
      int dify=cy-_dlgSize.cy;
      MoveWindowRel(dwp,wFileList,0,0,difx,dify);
      MoveWindowRel(dwp,wPopisek,0,dify,0,0);
      MoveWindowRel(dwp,wFolder,0,dify,difx,0);
      MoveWindowRel(dwp,wExport,difx,dify,0,0);
      MoveWindowRel(dwp,wBrowse,difx,dify,0,0);
      MoveWindowRel(dwp,wDDLFile,0,0,difx,0);
      MoveWindowRel(dwp,wDDLBrowse,difx,0,0,0);
      EndDeferWindowPos(dwp);

    }
  _dlgSize.cx=cx;
  _dlgSize.cy=cy;
  }
}    


void CDDLReaderDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
  CDialog::OnGetMinMaxInfo(lpMMI);
  lpMMI->ptMinTrackSize.x=200;
  lpMMI->ptMinTrackSize.y=150;
  
}

void CDDLReaderDlg::OnEnKillfocusDdlfile()
{
  CString name;
  wDDLFile.GetWindowText(name);
  if (_ddlfile.OpenDDLFile(WString(name.GetString()))==false)
    AfxMessageBox(IDS_FILEOPENFAILED);
  UpdateList();
}

static CString GetGroupName(int group)
{
  int id;
  switch (group)
  {
  case 1: id= IDC_GROUP_GRAPHICS;break;
  case 2: id= IDC_GROUP_SOUNDS;break;
  case 3: id= IDC_GROUP_FONTS;break;
  case 7: id= IDC_GROUP_BASIC;break;
  case 8: id= IDC_GROUP_ITEMS;break;
  case 9: id= IDC_GROUP_MONSTERS;break;
  case 11: id= IDC_GROUP_DIALOGS;break;
  case 0: id=IDC_GROUP_UNSPECIFIED;break;
  default: id=IDC_GROUP_UNKNOWN;break;
  }
  CString res;
  res.Format(id,group);
  return res;
}

bool CDDLReaderDlg::File(WString name, int group, uint32_t offset)
{
  LVITEM item;
  CString grpname=GetGroupName(group);
  wchar_t buff[40];
  item.iItem=wFileList.GetItemCount();
  item.iSubItem=0;
  item.mask=LVIF_GROUPID|LVIF_TEXT;
  item.iGroupId=group;
  item.pszText=grpname.LockBuffer();
  int ipos=wFileList.InsertItem(&item);
  wFileList.SetItemText(ipos,1,name);
  wFileList.SetItemText(ipos,3,_ui64tow(offset,buff,10));
  grpname.UnlockBuffer();
  return true;
}

void CDDLReaderDlg::UpdateList(void)
{
  CString tmp;
  wFileList.DeleteAllItems();
  _ddlfile.EnumFiles(*this);
  for (int i=0,cnt=wFileList.GetItemCount();i<cnt;i++)
  {
    uint32_t offset;
    tmp=wFileList.GetItemText(i,3);
    offset=_wtoi(tmp);
    uint32_t size=_ddlfile.GetFileSize(offset);
    if (size<1024)
      tmp.Format(_T("%d B"),size);
    else
      tmp.Format(_T("%d KB"),(size+512)/1024);
    wFileList.SetItemText(i,2,tmp);
  }
}


void CDDLReaderDlg::OnBnClickedDdlbrowse()
{
  CString fname;
  wDDLFile.GetWindowText(fname);
  CString ddlfilter;
  ddlfilter.LoadString(IDS_DDLFILTER);
  CFileDialog fdlg(TRUE,_T("DDL"),fname,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,ddlfilter);
  if (fdlg.DoModal()==IDOK)
  {
    wDDLFile.SetWindowText(fdlg.GetPathName());
    OnEnKillfocusDdlfile();
  }
}

struct SortInfo
{
  int index;
  CListCtrl *list;
  CString left;
  CString right;
  DDLFile *_ddlfile;
};

static int CALLBACK SortItemsInFileList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  SortInfo &sinfo=*(SortInfo *)lParamSort;
  int res;
  if (sinfo.index!=2)
  {
    sinfo.left=sinfo.list->GetItemText(lParam1,sinfo.index);
    sinfo.right=sinfo.list->GetItemText(lParam2,sinfo.index);
    switch (sinfo.index)
    {
    case 0:
    case 1: res=wcsicmp(sinfo.left,sinfo.right);break;
    case 3: {
              int l=_wtoi(sinfo.left);
              int r=_wtoi(sinfo.right);
              res=(l>r)-(l<r);
            }
    }
  }
  else
  {
    sinfo.left=sinfo.list->GetItemText(lParam1,3);
    sinfo.right=sinfo.list->GetItemText(lParam2,3);
    uint32_t l=_wtoi(sinfo.left);
    uint32_t r=_wtoi(sinfo.right);
    l=sinfo._ddlfile->GetFileSize(l);
    r=sinfo._ddlfile->GetFileSize(r);
    res=(l>r)-(l<r);
  }
  if (res==0) res=(lParam1>lParam2)-(lParam1<lParam2);
  return res;
}



void CDDLReaderDlg::OnLvnColumnclickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  for (int i=0,cnt=wFileList.GetItemCount();i<cnt;i++)
    wFileList.SetItemData(i,i);
  SortInfo sinfo;
  sinfo.index=pNMLV->iSubItem;
  sinfo.list=&wFileList;
  sinfo._ddlfile=&_ddlfile;
  wFileList.SortItems(SortItemsInFileList,(LPARAM)&sinfo);
  *pResult = 0;
}

static int WINAPI PosBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
  {
  const wchar_t *curpath=(const wchar_t *)lpData;  
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
      wchar_t buff[_MAX_PATH];
      if (SHGetPathFromIDList((LPITEMIDLIST)lParam,buff))
        {        
        ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)buff);
        }
      }
  return 0;
  };

static bool PathBrowser(HWND hWnd, wchar_t *path /* MAX_PATH size */)
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


void CDDLReaderDlg::OnBnClickedBrowse()
{
  wchar_t path[MAX_PATH];
  wFolder.GetWindowText(path,MAX_PATH);
  if (PathBrowser(*this,path))
  {
    wFolder.SetWindowText(path);
  }
}

void CDDLReaderDlg::OnBnClickedExport()
{
  wchar_t path[MAX_PATH];
  wFolder.GetWindowText(path,MAX_PATH);
  if (path[0]==0) OnBnClickedBrowse();
  wFolder.GetWindowText(path,MAX_PATH);
  if (path[0]==0) return;
  WPathname fpath;
  fpath.SetDirectory(path);
  POSITION pos=wFileList.GetFirstSelectedItemPosition();
  int max=wFileList.GetSelectedCount();
  int cur=0;
  DlgProgress pb;  
  if (max) {pb.Create(IDD_EXPORTING);pb.CenterWindow(this);EnableWindow(FALSE);}
  while (pos)
  {    
    MSG msg;
    pb.wProgress.SetRange(0,max);
    pb.wProgress.SetPos(++cur);
    int i=wFileList.GetNextSelectedItem(pos);
    CString fname;
    uint32_t offset;
    fname=wFileList.GetItemText(i,1);
    offset=(uint32_t)_wtoi64(wFileList.GetItemText(i,3));
    pb.wDesc.SetWindowText(fname);
    if (PeekMessage(&msg,0,0,0,PM_NOREMOVE)==TRUE) AfxPumpMessage();
    if (pb.stop) break;
    DDLData data=_ddlfile.ExtractFile(offset);
    if (data.data!=NULL)
    {
      fpath.SetFilename(fname);
      HANDLE hFile=CreateFile(fpath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
      while (hFile==INVALID_HANDLE_VALUE)
      {
        CString msg;
        AfxFormatString1(msg,IDS_UNABLETOCREATEFILE,fpath);
        int retry=AfxMessageBox(msg,MB_ABORTRETRYIGNORE);
        if (retry==IDABORT) {EnableWindow(TRUE);return;}
        if (retry==IDIGNORE) break;
        hFile=CreateFile(fpath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
      }
      if (hFile!=INVALID_HANDLE_VALUE)
      {
        DWORD written;
        WriteFile(hFile,data.data,data.sz,&written,NULL);
        CloseHandle(hFile);
      }
    }
    else
    {
        CString msg;
        AfxFormatString1(msg,IDS_UNABLETOEXTACTDATA,fpath);
        AfxMessageBox(msg,MB_OK|MB_ICONSTOP);
    }
  }
  EnableWindow(TRUE);


}

WPathname CDDLReaderDlg::CreateTemp(int index)
{
  if (!_lastTemp.IsNull()) DeleteFile(_lastTemp);
  CString fname;
  uint32_t offset;
  fname=wFileList.GetItemText(index,1);
  offset=(uint32_t)_wtoi64(wFileList.GetItemText(index,3));
  _lastTemp.SetTempDirectory();
  _lastTemp.SetFileTitle(WSC("SkeldalDDLReader"));
  _lastTemp.SetExtension(WSC(".")+WString(fname));
  DDLData data=_ddlfile.ExtractFile(offset);
  if (data.data)
  {
      HANDLE hFile=CreateFile(_lastTemp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
      DWORD written;
      WriteFile(hFile,data.data,data.sz,&written,NULL);
      CloseHandle(hFile);
  }
  return _lastTemp;
}

void CDDLReaderDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
  // TODO: Add your control notification handler code here
  *pResult = 0;
  int index=wFileList.GetNextItem(-1,LVNI_FOCUSED);
  WPathname pth=CreateTemp(index);
  ShellExecute(*this,0,pth,0,0,SW_NORMAL);
}
