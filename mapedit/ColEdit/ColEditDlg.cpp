// ColEditDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ColEdit.h"
#include "ColEditDlg.h"
#include "..\..\LIBS\PCX.H"
#include "EditPaletteDlg.h"
#include "../../DDLReader/WString.h"

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
// CColEditDlg dialog

CColEditDlg::CColEditDlg(CWnd* pParent /*=NULL*/)
: CDialog(CColEditDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CColEditDlg)
  //}}AFX_DATA_INIT
  // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  _palused=0;
}

void CColEditDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CColEditDlg)
  DDX_Control(pDX, IDC_UP, wUp);
  DDX_Control(pDX, IDC_DOWN, wDown);
  DDX_Control(pDX, IDC_IMAGESEL, wImageSel);
  DDX_Control(pDX, IDC_IMAGE, wImage);
  DDX_Control(pDX, IDC_COLORLIST, wColList);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CColEditDlg, CDialog)
//{{AFX_MSG_MAP(CColEditDlg)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_LOAD, OnLoad)
ON_LBN_SELCHANGE(IDC_COLORLIST, OnSelchangeColorlist)
ON_CBN_SELCHANGE(IDC_IMAGESEL, OnSelchangeImagesel)
ON_BN_CLICKED(IDC_INSERT, OnInsert)
ON_BN_CLICKED(IDC_UP, OnUp)
ON_BN_CLICKED(IDC_DOWN, OnDown)
ON_BN_CLICKED(IDC_DUPLICATE, OnDuplicate)
ON_BN_CLICKED(IDC_DELETE, OnDelete)
ON_BN_CLICKED(IDC_OPRAVPALETU, OnOpravpaletu)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColEditDlg message handlers

BOOL CColEditDlg::OnInitDialog()
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
  
  wUp.SetIcon(AfxGetApp()->LoadIcon(IDI_UP));
  wDown.SetIcon(AfxGetApp()->LoadIcon(IDI_DOWN));
  
  // TODO: Add extra initialization here
  OnSelchangeColorlist();
  
  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CColEditDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CColEditDlg::OnPaint() 
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
HCURSOR CColEditDlg::OnQueryDragIcon()
{
  return (HCURSOR) m_hIcon;
}

void CColEditDlg::UpdateLB()
{
  CString name;
  wColList.ResetContent();
  for (int i=0;i<_palused;i++)
  {
	name.Format(IDS_PALETANAME,_indexes[i]);
	wColList.AddString(name);
  }
  if (_palused<MAX_PALET)
  {
	name.LoadString(IDS_NOVAPALETA);
	wColList.AddString(name);
  }
  
}

void CColEditDlg::OnLoad() 
{
  CString filter;
  filter.LoadString(IDS_COLFILTER);
  CFileDialog fdlg(FALSE, NULL, NULL, OFN_HIDEREADONLY, filter);
  if (fdlg.DoModal()==IDOK)
  {
	if (!LoadDocument(fdlg.GetPathName()))
	{
	  AfxMessageBox(IDS_LOADFAILED);
	  return;
	}
	
  }
  OnSelchangeColorlist();
}

bool CColEditDlg::LoadDocument(LPCTSTR name)
{
  int i;
  HANDLE f=CreateFile(name,GENERIC_READ,0,NULL,OPEN_ALWAYS,0,NULL);
  if (f==NULL) return false;
  DWORD sz=GetFileSize(f,NULL);
  _palused=sz/(768+8);
  for (i=0;i<_palused;i++)
  {
	DWORD temp[2];
	DWORD read;
	ReadFile(f,&temp,8,&read,NULL);
	if (read!=8) return false;
	ReadFile(f,_palety[i],768,&read,NULL);
	if (read!=768) return false;	
  }
  for (i=0;i<_palused;i++) _indexes[i]=i+1;
  CloseHandle(f);
  UpdateLB();  
  fname=name;
  UpdateCB();
  return true;
}

void CColEditDlg::UpdateCB()
{
  CString mask=fname.Mid(0,fname.GetLength()-4);
  mask=mask+_T("*.PCX");
  CFileFind fnd;
  wImageSel.ResetContent();
  BOOL ok;
  if (fnd.FindFile(mask)==TRUE) do
  {
	ok=fnd.FindNextFile();
	wImageSel.AddString(fnd.GetFileName());
  }
  while (ok);
  wImageSel.SetCurSel(0);
  OnSelchangeImagesel();
}


void CColEditDlg::OnSelchangeColorlist() 
{
  OnSelchangeImagesel();
  int curSel=wColList.GetCurSel();
  GetDlgItem(IDC_INSERT)->EnableWindow(curSel>=0);
  GetDlgItem(IDC_DUPLICATE)->EnableWindow(curSel>=0);
  GetDlgItem(IDC_DELETE)->EnableWindow(curSel>=0);
  GetDlgItem(IDC_UP)->EnableWindow(curSel>=0);
  GetDlgItem(IDC_DOWN)->EnableWindow(curSel>=0);
  GetDlgItem(IDC_OPRAVPALETU)->EnableWindow(curSel>=0);
}

extern "C"
{
  
  void *getmem(int32_t sz)
  {
	return malloc(sz);
  }
  
}

void CColEditDlg::OnSelchangeImagesel() 
{
  int i=wImageSel.GetCurSel();	
  if (i!=-1)
  {
	CString pos;
	wImageSel.GetLBText(i,pos);
	int bsl=fname.ReverseFind('\\');
	pos=fname.Mid(0,bsl+1)+pos;
	char * buffer;
	memset(_stbuff,0,sizeof(_stbuff));
#ifdef _UNICODE
    WString pname(pos.GetString());
    const char *pcxname=pname.AsUTF8(pname);
#else
	const char *pcxname=pos;
#endif
	if (open_pcx(const_cast<char *>(pcxname),A_8BIT,&buffer)) return;
	unsigned short *xy=(unsigned short *)buffer;
	for (int y=0;y<xy[1];y++)
	{
	  memcpy(_stbuff+y*640,buffer+6+512+xy[0]*y,min(480,xy[0]));
	}
	int j=wColList.GetCurSel();
	if (j<0) j=0;
	wImage.SetPalette(_palety[j]);
	wImage.UpdateBuffer(_stbuff);
	free(buffer);
  }	
}

void CColEditDlg::OnInsert() 
{
  CString filter;
  filter.LoadString(IDS_PALFILTER);
  CFileDialog fdlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, filter);
  if (fdlg.DoModal()==IDOK)
  {
	TCHAR buff[256];
	FILE *f=_tfopen(fdlg.GetPathName(),_T("r"));
	_fgetts(buff,256,f);
	bool ok=false;
	if (_tcscmp(buff,_T("JASC-PAL\n"))==0)
	{
	  _fgetts(buff,256,f);
	  if (_tcscmp(buff,_T("0100\n"))==0)
	  {
		_fgetts(buff,256,f);
		if (_tcscmp(buff,_T("256\n"))==0)
		{
		  int palid=DuplicateCurrent();
      if (palid>=0)      
      {      
		    char *p=_palety[palid];
		    for (int i=0;i<768;i++)
		    {
			  int item=0;
			  _ftscanf(f,_T("%d"),&item);
			  *p++=(char)item;
		    }
		    ok=true;
		    OnSelchangeImagesel();
      }
    }
	  }
	}
	fclose(f);
	if (!ok)
	  AfxMessageBox(IDS_LOADFAILED,MB_OK);
  }	
}

int CColEditDlg::DuplicateCurrent()
{ 
  int i;
  int curSel=wColList.GetCurSel();
  int saveCurSel=curSel;
  if (curSel==_palused) curSel--;
  if (curSel==-1) curSel=0;
  for (i=MAX_PALET-1;i>curSel;i--)
  {
	memcpy(_palety[i],_palety[i-1],sizeof(_palety[i]));
	_indexes[i]=_indexes[i-1];
  }
  _palused++;
  if (_palused>MAX_PALET) _palused=MAX_PALET;
  int maxnum=0;
  for (i=0;i<_palused;i++)
	if (_indexes[i]>maxnum) maxnum=_indexes[i];
	_indexes[saveCurSel]=maxnum+1;
	UpdateLB();
	wColList.SetCurSel(saveCurSel);
  return saveCurSel;
}


void CColEditDlg::OnUp() 
{
  int i;
  int curSel=wColList.GetCurSel();
  if (curSel<1) return;
  if (curSel>=_palused) return;
  i=curSel;
  char buff[768];
  memcpy(buff,_palety[i-1],sizeof(_palety[i]));
  memcpy(_palety[i-1],_palety[i],sizeof(_palety[i]));
  memcpy(_palety[i],buff,sizeof(_palety[i]));
  int p=_indexes[i-1];
  _indexes[i-1]=_indexes[i];
  _indexes[i]=p;
  curSel--;
  UpdateLB();
  wColList.SetCurSel(curSel);	
}

void CColEditDlg::OnDown() 
{
  int i;
  int curSel=wColList.GetCurSel();
  if (curSel<0) return;
  if (curSel>=_palused-1) return;
  i=curSel;
  char buff[768];
  memcpy(buff,_palety[i+1],sizeof(_palety[i]));
  memcpy(_palety[i+1],_palety[i],sizeof(_palety[i]));
  memcpy(_palety[i],buff,sizeof(_palety[i]));
  int p=_indexes[i+1];
  _indexes[i+1]=_indexes[i];
  _indexes[i]=p;
  curSel++;
  UpdateLB();
  wColList.SetCurSel(curSel);	
  
}

void CColEditDlg::OnDuplicate() 
{
  DuplicateCurrent();	
  OnSelchangeImagesel();

}

void CColEditDlg::OnDelete() 
{
  int i=wColList.GetCurSel();
  if (i<0) return;
  if (i>=_palused) return;
  for (int p=i+1;p<_palused;p++)
  {
	memcpy(_palety[p-1],_palety[p],sizeof(_palety[p]));
	_indexes[p-1]=_indexes[p];
  }
  _palused--;
  UpdateLB();  
  OnSelchangeColorlist();
}

void CColEditDlg::OnOK() 
{
  if (fname=="")
  {
	CString filter;
	filter.LoadString(IDS_COLFILTER);
	CFileDialog fdlg(FALSE,_T("COL"),fname,OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT,filter);
	if (fdlg.DoModal()!=IDOK) return;
	fname=fdlg.GetPathName();
  }
  if (SaveDocument(fname)==false)
	AfxMessageBox(IDS_SAVEFAILED);
  
  
}

bool CColEditDlg::SaveDocument(LPCTSTR name)
{
  HANDLE h=CreateFile(name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
  if (h==INVALID_HANDLE_VALUE) return false;
  for (int i=0;i<_palused;i++)
  {
	DWORD dummy[2]={0,0};
	DWORD result=0;
	WriteFile(h,&dummy,8,&result,NULL);
	if (result!=8) {CloseHandle(h);return false;}
	WriteFile(h,_palety[i],768,&result,NULL);
	if (result!=768) {CloseHandle(h);return false;}
  }
  CloseHandle(h);
  return true;
}

void CColEditDlg::OnOpravpaletu() 
{
  CEditPaletteDlg	dlg;
  int i=wColList.GetCurSel();
  if (i<0) return;
  dlg._palette=_palety[i];
  dlg._imgView=&wImage;
  dlg.DoModal();
  
}
