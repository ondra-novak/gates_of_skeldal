// ItemIconsDlg.cpp : implementation file
//


#include "stdafx.h"
#include "ItemIcons.h"
#include "ItemIconsDlg.h"
#include ".\itemiconsdlg.h"
#include "DlgOpen.h"
#include <commdlg.h>

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
public:
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


// CItemIconsDlg dialog



CItemIconsDlg::CItemIconsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CItemIconsDlg::IDD, pParent)
    , _selicon(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CItemIconsDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  for (int i=0;i<18;i++)
    DDX_Control(pDX, IDC_ICON1+i, icons[i]);
  DDX_Control(pDX, IDC_FILENAME, wFilename);
}

BEGIN_MESSAGE_MAP(CItemIconsDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_OPEN, OnOpen)
  ON_WM_CONTEXTMENU()
  ON_COMMAND(ID_POPUP_PASTE, OnPopupPaste)
  ON_COMMAND(ID_POPUP_COPY, OnPopupCopy)
  ON_COMMAND(ID_POPUP_IMPORTBMP, OnPopupImportbmp)
  ON_WM_DROPFILES()
  ON_COMMAND(ID_POPUP_EXPORTBMP, OnPopupExportbmp)
  ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
  ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
  ON_COMMAND_RANGE(IDC_ICON1,IDC_ICON18,OnClickIcon)
  ON_COMMAND(ID_EDIT_EXPORT, OnEditExport)
  ON_COMMAND(ID_EDIT_IMPORT, OnEditImport)
  ON_COMMAND(ID_EDIT_EXPORTALL, OnEditExportall)
  ON_COMMAND(ID_SAVE, OnSave)
  ON_COMMAND(ID_POPUP_VYMAZAT, OnPopupVymazat)
  ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
END_MESSAGE_MAP()


// CItemIconsDlg message handlers

BOOL CItemIconsDlg::OnInitDialog()
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
	ZeroMemory(_iconlib,sizeof(_iconlib));
    for (int i=0;i<18;i++) icons[i]._data=_iconlib+i*ICONSIZE;
    CheckDlgButton(IDC_TRANSPARENT,1);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CItemIconsDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CItemIconsDlg::OnPaint() 
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
HCURSOR CItemIconsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CItemIconsDlg::OnOpen()
{  
  if (_dlgOpen.DoModal()==IDOK)
  {
    CString name=_dlgOpen.GetSelectedFile();
    LoadDocument(name);
  }
}

bool CItemIconsDlg::LoadDocument(const CString & name)
{
  _documentName=name;
  wFilename.SetWindowText(name);
  HANDLE h=CreateFile(name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
  if (h==INVALID_HANDLE_VALUE)
  {
    ZeroMemory(_iconlib,sizeof(_iconlib));
    for (int i=0;i<18;i++)
    {
      unsigned short *data=reinterpret_cast<unsigned short *>(_iconlib+i*ICONSIZE);
      data[0]=45;
      data[1]=55;
      data[2]=8;
    }
  }
  else
  {
    DWORD rd;
    ReadFile(h,_iconlib,ICONLIBSIZE,&rd,0);
    CloseHandle(h);
  }
  for (int i=0;i<18;i++) icons[i].Invalidate(TRUE);
  return true;
}

void CItemIconsDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
  _selicon=IconFromWnd(pWnd);
  if (_selicon!=-1)
  {
    CMenu mnu;
    mnu.LoadMenu(IDR_POPUP);
    CMenu *sub=mnu.GetSubMenu(0);
    sub->TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this,0);
  }
}

int CItemIconsDlg::IconFromWnd(CWnd * wnd)
{
  for (int i=0;i<18;i++) if (icons+i==wnd) return i;
  return -1;
}

int CItemIconsDlg::ImportBMP(int icon,void *bmp, int dataoffset)
{
  BITMAPINFO *binfo=reinterpret_cast<BITMAPINFO *>(bmp);  
  char *data=reinterpret_cast<char *>(bmp)+dataoffset;
  if (binfo->bmiHeader.biBitCount==8 && binfo->bmiHeader.biWidth==45 && binfo->bmiHeader.biHeight==55 && binfo->bmiHeader.biCompression==BI_RGB)
  {
    unsigned short *pal=GETICONPAL(icon);
    char *bits=GETICONBITS(icon);
    for (int i=0;i<256;i++) 
      pal[i]=(((unsigned)binfo->bmiColors[i].rgbRed>>3)<<10)|
              (((unsigned)binfo->bmiColors[i].rgbGreen>>3)<<5)|
              (((unsigned)binfo->bmiColors[i].rgbBlue>>3));
    for (int i=0;i<55;i++,data+=3)
    {
      bits=GETICONBITS(icon)+(55-i-1)*45;
      for (int j=0;j<45;j++)
        *bits++=*data++;
    }
    if (IsDlgButtonChecked(IDC_TRANSPARENT))  AutodetectTransparent(icon);
  }
  else
    AfxMessageBox(IDS_CHYBNYFORMAT);
  icons[icon].Invalidate();
  return 0;
}

void CItemIconsDlg::OnPopupPaste()
{
  OpenClipboard();
  HANDLE h=GetClipboardData(CF_DIB);
  if (h)
  {
    LPVOID v=GlobalLock(h);
    ImportBMP(_selicon,v,sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256);
    GlobalUnlock(h);
  }
  CloseClipboard();
}

void CItemIconsDlg::OnPopupCopy()
{
  HANDLE buff=GlobalAlloc(GHND, sizeof(BitMapInfo)+48*55);
  BITMAPINFO *bmp=reinterpret_cast<BITMAPINFO *>(GlobalLock(buff));
  ExportBMP(_selicon,bmp);
  GlobalUnlock(buff);
  OpenClipboard();
  EmptyClipboard();
  SetClipboardData(CF_DIB,buff);
  CloseClipboard();
}

void CItemIconsDlg::ExportBMP(int icon, BITMAPINFO * bitmap)
{
  BitMapInfo &bmpinfo=*reinterpret_cast<BitMapInfo *>(bitmap);

  ZeroMemory(&bmpinfo,sizeof(bmpinfo));
  bmpinfo.hdr.biSize=sizeof(bmpinfo.hdr);
  bmpinfo.hdr.biBitCount=8;
  bmpinfo.hdr.biCompression=BI_RGB;
  bmpinfo.hdr.biHeight=55;
  bmpinfo.hdr.biPlanes=1;
  bmpinfo.hdr.biWidth=45; 
  unsigned short *pal=GETICONPAL(icon);
  for (int i=0;i<256;i++) 
  {
    RGBQUAD &col=bmpinfo.palette[i];
    col.rgbBlue=(pal[i] & 0x1F)<<3;
    col.rgbGreen=(pal[i] & 0x3E0)>>2;
    col.rgbRed=(pal[i] & 0x7C00)>>7;
    col.rgbReserved=0;
  }  
  char *input=GETICONBITS(icon);
  for (int i=0;i<55;i++)
  {
    char *data=reinterpret_cast<char *>(bitmap)+sizeof(bmpinfo)+48*(54-i);  
    for (int j=0;j<45;j++)
      *data++=*input++;
  }

}

void CItemIconsDlg::OnPopupImportbmp()
{
  CString filter;
  filter.LoadString(IDS_BMPFILTER);
  CFileDialog fdlg(TRUE,0,0,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,filter,0);
  if (fdlg.DoModal()==IDOK)
  {
    ImportBMP(fdlg.GetPathName(),_selicon);
  }
}

void CItemIconsDlg::OnDropFiles(HDROP hDropInfo)
{
  CPoint mouse;
  DragQueryPoint(hDropInfo,&mouse);
//  ScreenToClient(&mouse);
  CWnd *wnd=ChildWindowFromPoint(mouse,CWP_SKIPINVISIBLE|CWP_SKIPDISABLED);
  int icon=IconFromWnd(wnd);
  if (icon!=-1)
  {
    int index=0;
    CString name;
    int sz=DragQueryFile(hDropInfo,index,0,0);
    while (sz)
    {
      _TCHAR *buff=name.GetBuffer(sz+1);
      DragQueryFile(hDropInfo,index,buff,sz+1);
      _TCHAR *ext=_tcsrchr(buff,'.');
      if (ext && _tcsicmp(ext,_T(".BMP"))==0)      
        ImportBMP(buff,icon);
      name.ReleaseBuffer();
      icon++;
      if (icon>=18) icon=0;
      index++;
      sz=DragQueryFile(hDropInfo,index,0,0);
    }
  }
  DragFinish(hDropInfo);
}

void CItemIconsDlg::ImportBMP(const _TCHAR * name, int icon)
{
    HANDLE h=CreateFile(name,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
    if (h!=INVALID_HANDLE_VALUE)
    {
      BITMAPFILEHEADER *hdr=(BITMAPFILEHEADER *)malloc(GetFileSize(h,0));
      DWORD rd;
      ReadFile(h,hdr,GetFileSize(h,0),&rd,0);
      if (hdr->bfType==MAKEWORD('B','M'))
      {
        BITMAPINFO *nfo=(BITMAPINFO *)((char *)hdr+sizeof(BITMAPFILEHEADER));
        ImportBMP(icon,nfo,hdr->bfOffBits-sizeof(*hdr));
      }
      else
        AfxMessageBox(IDS_NEPLATNYFORMATBMP);
      free(hdr);
      CloseHandle(h);
    }
    else
      AfxMessageBox(IDS_NELZEOTEVRITSOUBOR);
}

void CItemIconsDlg::ExportBMP(int icon, const _TCHAR * filename)
{
  DWORD sz=48*55+sizeof(BitMapInfo);
  char *data=(char *)malloc(sz);
  ExportBMP(icon,(BITMAPINFO *)data);
  BITMAPFILEHEADER hdr;
  ZeroMemory(&hdr,sizeof(hdr));
  hdr.bfType=MAKEWORD('B','M');
  hdr.bfSize=sz;
  hdr.bfOffBits=sizeof(hdr)+sizeof(BitMapInfo);
  HANDLE h=CreateFile(filename,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
  if (h!=INVALID_HANDLE_VALUE)
  {
    DWORD rd;
    WriteFile(h,&hdr,sizeof(hdr),&rd,0);
    if (rd!=sizeof(hdr))
    {
      AfxMessageBox(IDS_CHYBAPRIZAPISU);
    }
    else 
    {
      WriteFile(h,data,sz,&rd,0);
      if (sz!=rd)
      {
        AfxMessageBox(IDS_CHYBAPRIZAPISU);
      }
    }
    CloseHandle(h);
  }
  else
      AfxMessageBox(IDS_NELZEOTEVRITSOUBOR);
}

void CItemIconsDlg::OnPopupExportbmp()
{
  CString filter;
  filter.LoadString(IDS_BMPFILTER);
  CFileDialog fdlg(FALSE,_T(".BMP"),0,OFN_HIDEREADONLY|OFN_PATHMUSTEXIST,filter,0);
  if (fdlg.DoModal()==IDOK)
  {
    ExportBMP(_selicon,fdlg.GetPathName());
  }
}

int CItemIconsDlg::ManualSelectPic(void)
{
  CMenu mnu;
  mnu.LoadMenu(IDR_POPUP);
  CMenu *sub=mnu.GetSubMenu(1);
  CPoint pt(0,0);
  ClientToScreen(&pt);
  int res=sub->TrackPopupMenu(TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,this,0);
  return res-1;
}

void CItemIconsDlg::OnEditCopy()
{
  _selicon=ManualSelectPic();
  if (_selicon>=0) OnPopupCopy();
}

void CItemIconsDlg::OnEditPaste()
{
  _selicon=ManualSelectPic();
  if (_selicon>=0) OnPopupPaste();
}

void CItemIconsDlg::OnClickIcon(UINT cmd)
{
  _selicon=cmd-IDC_ICON1;
 OnPopupImportbmp();
}
void CItemIconsDlg::OnEditExport()
{
  _selicon=ManualSelectPic();
  if (_selicon>=0) OnPopupExportbmp();
}

void CItemIconsDlg::OnEditImport()
{
  CString filter;
  filter.LoadString(IDS_BMPFILTER);
  CFileDialog fdlg(TRUE,0,0,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT,filter,0);
  if (fdlg.DoModal()==IDOK)
  {
    _selicon=ManualSelectPic();
    POSITION pos=fdlg.GetStartPosition();
    while (pos)
    {      
      ImportBMP(fdlg.GetNextPathName(pos),_selicon);
      _selicon=(_selicon+1)%18;
    }
  }
}

void CItemIconsDlg::OnEditExportall()
{
  CString filter;
  filter.LoadString(IDS_BMPFILTER);
  CFileDialog fdlg(FALSE,0,0,OFN_HIDEREADONLY|OFN_PATHMUSTEXIST,filter,0);
  if (fdlg.DoModal()==IDOK)
  {
    CString mask=fdlg.GetPathName()+_T(".%02d.BMP");
    for (int i=0;i<18;i++)
    {
      CString name;
      name.Format(mask,i+1);
      ExportBMP(i,name);
    }
  }
}

void CItemIconsDlg::OnSave()
{
  if (SaveDocument(_documentName)==false)
  {
    AfxMessageBox(IDS_CHYBAPRIULOZENI);
  }
}

bool CItemIconsDlg::SaveDocument(const _TCHAR * name)
{
  HANDLE h=CreateFile(name,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,0,CREATE_ALWAYS,FILE_FLAG_SEQUENTIAL_SCAN,0);
  if (h==INVALID_HANDLE_VALUE)
  {
    return false;
  }
  else
  {
    DWORD rd;
    WriteFile(h,_iconlib,ICONLIBSIZE,&rd,0);   
    CloseHandle(h);
    return rd==ICONLIBSIZE;
  }

}

void CItemIconsDlg::OnPopupVymazat()
{
  char *bits=GETICONBITS(_selicon);
  ZeroMemory(bits,45*55);
  icons[_selicon].Invalidate();
}

void CItemIconsDlg::OnEditDelete()
{
  _selicon=ManualSelectPic();
  if (_selicon>=0) OnPopupVymazat();
  
}

void CItemIconsDlg::SetTransparentIndex(int icon, int index)
{
  if (index==0) return;
  unsigned short *pal=GETICONPAL(icon);
  char *bits=GETICONBITS(icon);
  unsigned short save=pal[index];
  pal[index]=pal[0];
  pal[0]=save;

  for (int i=0;i<45*55;i++,bits++) if (*bits==index) *bits=0;else if (*bits==0) *bits=index;
}


void CItemIconsDlg::AutodetectTransparent(int icon)
{
  int stats[256];
  ZeroMemory(stats,sizeof(stats));
  for (int i=0;i<45;i++)
  {
    stats[*(unsigned char *)(GETICONBITS(icon)+i)]++;
    stats[*(unsigned char *)(GETICONBITS(icon)+i+54*45)]++;
  }
  for (int i=1;i<54;i++)
  {
    stats[*(unsigned char *)(GETICONBITS(icon)+i*45)]++;
    stats[*(unsigned char *)(GETICONBITS(icon)+i*45+44)]++;
  }
  int max=0,index=0;
  for (int i=0;i<256;i++) if (stats[i]>max) {max=stats[i];index=i;}
  SetTransparentIndex(icon,index);
}
