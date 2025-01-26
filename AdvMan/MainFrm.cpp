// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "AdvMan.h"
#include "..\cztable.h"

#include "MainFrm.h"
#include "DlgDialogy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_FILE_NOVDOBRODRUST, OnFileNovdobrodrust)
	ON_COMMAND(ID_FILE_NATIDOBRODRUSTV, OnFileNatidobrodrustv)
	ON_COMMAND(ID_EDITORY_KOUZLATAB, OnEditoryKouzlatab)
	ON_COMMAND(ID_EDITORY_POSTAVYTAB, OnEditoryPostavytab)
	ON_COMMAND(ID_EDITORY_ITEMSSCR, OnEditoryItemsscr)
	ON_COMMAND(ID_EDITORY_ITEMSPIC, OnEditoryItemspic)
	ON_COMMAND(ID_EDITORY_WEAPONSSCR, OnEditoryWeaponsscr)
	ON_WM_DESTROY()
	ON_COMMAND(ID_PEKLADAE_PELOKOUZLA, OnPekladaePelokouzla)
	ON_COMMAND(ID_PEKLADAE_PELODIALOGY, OnPekladaePelodialogy)
	ON_COMMAND(ID_PEKLADAE_PELOPOSTAVYTAB, OnPekladaePelopostavytab)
	ON_COMMAND(ID_NSTROJE_MAPEDIT, OnNstrojeMapedit)
	ON_COMMAND(ID_NSTROJE_TESTUJDOBRODRUSTV, OnNstrojeTestujdobrodrustv)
	ON_COMMAND(ID_NSTROJE_TVRCEPODLAH, OnNstrojeTvrcepodlah)
	ON_COMMAND(ID_NSTROJE_TVRCEPALETPRONESTVRY, OnNstrojeTvrcepaletpronestvry)
	ON_COMMAND(ID_NSTROJE_TVRCEIKONPROPEDMTY, OnNstrojeTvrceikonpropedmty)
	ON_COMMAND(ID_EDITORY_SOUBORADV, OnEditorySouboradv)
	ON_COMMAND(ID_FILE_ZNOVUNAST, OnFileZnovunast)
	ON_UPDATE_COMMAND_UI(ID_FILE_ZNOVUNAST, OnUpdateFileZnovunast)
	ON_COMMAND(ID_EDITORY_DIALOGY, OnEditoryDialogy)
  ON_COMMAND_EX(ID_VIEW_EDITORY,OnBarCheck)
  ON_COMMAND_EX(ID_VIEW_PREKLADACE,OnBarCheck)
  ON_UPDATE_COMMAND_UI(ID_VIEW_EDITORY,OnUpdateControlBarMenu)
  ON_UPDATE_COMMAND_UI(ID_VIEW_PREKLADACE,OnUpdateControlBarMenu)
  ON_UPDATE_COMMAND_UI_RANGE(ID_EDITORY_KOUZLATAB,ID_EDITORY_WEAPONSSCR,OnUpdateEditory)
  ON_UPDATE_COMMAND_UI_RANGE(ID_PEKLADAE_PELOKOUZLA,ID_PEKLADAE_PELOPOSTAVYTAB,OnUpdateEditory)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here

}

CMainFrame::~CMainFrame()
{

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(ES_READONLY|WS_VISIBLE|WS_CHILD|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|WS_VSCROLL|WS_HSCROLL,CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_BUTTON, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_TOOLTIPS | CBRS_FLYBY |BTNS_AUTOSIZE | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

  if (!m_wndEditoryBar.CreateEx(this, TBSTYLE_BUTTON, WS_CHILD | WS_VISIBLE | CBRS_TOP
    | CBRS_TOOLTIPS | CBRS_FLYBY |BTNS_AUTOSIZE | CBRS_SIZE_DYNAMIC,CRect(0,0,0,0),ID_VIEW_EDITORY) ||
    !m_wndEditoryBar.LoadToolBar(IDR_TOOLEDITORY))
  {
    TRACE0("Failed to create toolbar\n");
    return -1;      // fail to create
  }

  if (!m_wndPrekladaceBar.CreateEx(this, TBSTYLE_BUTTON, WS_CHILD | WS_VISIBLE | CBRS_TOP
    | CBRS_TOOLTIPS | CBRS_FLYBY |BTNS_AUTOSIZE | CBRS_SIZE_DYNAMIC,CRect(0,0,0,0),ID_VIEW_PREKLADACE) ||
    !m_wndPrekladaceBar.LoadToolBar(IDR_PREKLADACE))
  {
    TRACE0("Failed to create toolbar\n");
    return -1;      // fail to create
  }

  if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
  ShowWindow(SW_SHOW);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_wndEditoryBar.EnableDocking(CBRS_ALIGN_ANY);
  m_wndPrekladaceBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
  DockControlBar(&m_wndEditoryBar);
  DockControlBar(&m_wndPrekladaceBar);
  UpdateWindow();
  CRect rc;
  GetClientRect(&rc);
  ClientToScreen(&rc);
  DockControlBar(&m_wndEditoryBar,(UINT)0,CRect(rc.left+180,rc.top+5,rc.left+180,rc.top+5));
  DockControlBar(&m_wndPrekladaceBar,(UINT)0,CRect(rc.left+350,rc.top+5,rc.left+350,rc.top+5));

	_adv=0;
	CString untitled;
	untitled.LoadString(IDS_UNTITLED);
	SetTitle(untitled);

	SetClassLong(*this,GCL_HICON,(LONG)(theApp.LoadIcon(IDR_MAINFRAME)));

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
  if (_adv==0)
  {
	for (int i=0,cnt=pPopupMenu->GetMenuItemCount();i<cnt;i++)
	{
	  int id=pPopupMenu->GetMenuItemID(i);
	  if (id!=ID_FILE_NOVDOBRODRUST && id!=ID_FILE_NATIDOBRODRUSTV && id!=ID_APP_EXIT &&
		id!=ID_VIEW_TOOLBAR && id!=ID_VIEW_STATUS_BAR && id!=ID_APP_ABOUT && id!=ID_VIEW_EDITORY)
		pPopupMenu->EnableMenuItem(i,MF_GRAYED|MF_BYPOSITION);
	}
  }
  else
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}

#include "DlgNoveDobr.h"

static str_add_wildcard(TSTR_LIST *lst, const char *mask)
{
  CFileFind fnd;
  BOOL nxt=fnd.FindFile(mask);
  if (nxt==FALSE)
  {
	CString msg;
	AfxFormatString1(msg,IDS_NEMOHUNAJITNICOD,mask);
	AfxMessageBox(msg,MB_ICONEXCLAMATION);
  }
  while (nxt)
  {
	nxt=fnd.FindNextFile();
	str_add(lst,fnd.GetFilePath());
  }
}

void CMainFrame::OnFileNovdobrodrust()
{
  DlgNoveDobr dlg;
  int id=dlg.DoModal();
  if (id==IDOK)
  {
	release_list(_adv);
	_adv=create_list(10);
	CString baseMap=_T("adv\\")+dlg.vJmeno+_T("\\");
	add_field_txt(&_adv,_T("CESTA_MAPY"),baseMap);
	CString cestaGrafika;
	CString cestaDialogy;
	CString cestaEnemy;
	CString cestaItemy;
	CString cestaPozice;
	CString cestaZvuky;
	CString cestaBasicGr;
	switch (dlg.vOrganizace)
	{
	case 0:
	  cestaGrafika=baseMap+_T("GRFSTENY" PATH_SEPARATOR);
	  cestaDialogy=baseMap+_T("GRFDIALG" PATH_SEPARATOR);
	  cestaEnemy=baseMap+_T("GRFENEMY" PATH_SEPARATOR);
	  cestaItemy=baseMap+_T("GRFITEMS" PATH_SEPARATOR);
	  cestaPozice=baseMap+_T("SAVEGAME" PATH_SEPARATOR);
	  cestaZvuky=baseMap+_T("SOUNDS" PATH_SEPARATOR);
	  cestaBasicGr=baseMap+_T("GRFBASIC" PATH_SEPARATOR);
	  break;
	case 1:
	  cestaGrafika=baseMap+_T("graphics" PATH_SEPARATOR);
	  cestaDialogy=baseMap+_T("graphics" PATH_SEPARATOR "dialogs" PATH_SEPARATOR);
	  cestaEnemy=baseMap+_T("graphics" PATH_SEPARATOR "enemies" PATH_SEPARATOR);
	  cestaItemy=baseMap+_T("graphics" PATH_SEPARATOR "items");
	  cestaPozice=baseMap+_T("SAVEGAME" PATH_SEPARATOR );
	  cestaZvuky=baseMap+_T("SAMPLES" PATH_SEPARATOR);
	  cestaBasicGr=baseMap+_T("graphics" PATH_SEPARATOR "basic" PATH_SEPARATOR);
	  break;
	case 2:
	  cestaGrafika=baseMap;
	  cestaDialogy=baseMap;
	  cestaEnemy=baseMap;
	  cestaItemy=baseMap;
	  cestaPozice=baseMap+_T("SAVEGAME" PATH_SEPARATOR);
	  cestaZvuky=baseMap;
	  cestaBasicGr=baseMap;
	  break;
	}

	add_field_txt(&_adv,_T("CESTA_GRAFIKA"),cestaGrafika);
	add_field_txt(&_adv,_T("CESTA_DIALOGY"),cestaDialogy);
	add_field_txt(&_adv,_T("CESTA_ENEMY"),cestaEnemy);
	add_field_txt(&_adv,_T("CESTA_ITEMY"),cestaItemy);
	add_field_txt(&_adv,_T("CESTA_POZICE"),cestaPozice);
	add_field_txt(&_adv,_T("CESTA_ZVUKY"),cestaZvuky);
	add_field_txt(&_adv,_T("CESTA_BGRAFIKA"),cestaBasicGr);

	add_field_txt(&_adv,_T("DEFAULT_MAP"),dlg.vStartMap);
	add_field_num(&_adv,_T("CHAR_MIN"),dlg.vMinPostav);
	add_field_num(&_adv,_T("CHAR_MAX"),dlg.vMaxPostav);
	add_field_num(&_adv,_T("PATCH"),1);

	Pathname pth=Pathname::GetExePath();
	pth.SetFiletitle(dlg.vJmeno);
	pth.SetExtension(_T(".adv"));

	save_config(_adv,pth);

	_advName=pth;
	_advPath=pth.GetDirectoryWithDrive();

	SetTitle(pth);


	Pathname exePath=Pathname::GetExePath();

	pth.CreateFolder(exePath.GetDirectoryWithDrive()+baseMap);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaGrafika);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaDialogy);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaItemy);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaEnemy);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaPozice);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaZvuky);
	pth.CreateFolder(exePath.GetDirectoryWithDrive()+cestaBasicGr);

	CString basePath=pth.GetDirectoryWithDrive();

	TSTR_LIST fileList=create_list(10);
	str_add_wildcard(&fileList,basePath+_T("maps\\*.dat"));
	if (!dlg.vDefinice)	str_add_wildcard(&fileList,basePath+_T("AdvManData\\weapons.scr"));
	if (!dlg.vDefinice)	str_add_wildcard(&fileList,basePath+_T("AdvManData\\items.scr"));
	str_add_wildcard(&fileList,basePath+_T("AdvManData\\items.pic"));
	str_add_wildcard(&fileList,basePath+_T("AdvManData\\*.lst"));
    str_add_wildcard(&fileList,basePath+_T("AdvManData\\postavy.def"));
	str_add_wildcard(&fileList,basePath+_T("AdvManData\\postavy.tab"));
	str_add_wildcard(&fileList,basePath+_T("AdvManData\\predmety.tab"));
	if (dlg.vKouzla)
	{
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\kouzla.def"));
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\kouzla.tab"));
	}
	if (dlg.vDialogy)
	{
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\dialogy.def"));
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\dlgsecrt.def"));
	}
	if (dlg.vDefinice)
	{
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\*.scr"));
	  str_add_wildcard(&fileList,basePath+_T("AdvManData\\*.txt"));
	}
	if (dlg.vMapy)
	{
	  str_add_wildcard(&fileList,basePath+_T("maps\\*.map"));
	}
  if (dlg.vDialogyDlg)
  {
    str_add_wildcard(&fileList,basePath+_T("AdvManData\\*.dlg"));
  }
	int totalszneed=0;
	int i,cnt;
	for (i=0,cnt=str_count(fileList);i<cnt;i++)
	{
	  const char *file=fileList[i];
	  if (file) totalszneed+=_tcslen(file)+1;
	}

	char *buffer=(char *)alloca((totalszneed+1)*sizeof(*buffer));
	char *ptr=buffer;
	for (i=0,cnt=str_count(fileList);i<cnt;i++)
	{
	  const char *file=fileList[i];
	  if (file)
	  {
		strcpy(ptr,file);
		ptr=_tcschr(ptr,0)+1;
	  }
	}
	*ptr=0;


	pth.SetDirectory(exePath.GetDirectoryWithDrive()+baseMap);
	CString target;
	pth.GetDirectoryWithDriveWLBS(target.GetBuffer(_tcslen(pth.GetDirectoryWithDrive())),_tcslen(pth.GetDirectoryWithDrive()));
	target.ReleaseBuffer();

	SHFILEOPSTRUCT oper;
	memset(&oper,0,sizeof(oper));
	oper.hwnd=*this;
	oper.wFunc=FO_COPY;
	oper.pFrom=buffer;
	oper.pTo=target;
	oper.fFlags=0;
	SHFileOperation(&oper);
	release_list(fileList);

	_baseMapPath=_advPath+baseMap;

	CFileFind fnd;
	BOOL f=fnd.FindFile(_baseMapPath+"*.*");
	while (f)
	{
	  f=fnd.FindNextFile();
	  SetFileAttributes(fnd.GetFilePath(),GetFileAttributes(fnd.GetFilePath()) &~FILE_ATTRIBUTE_READONLY);
	}

	AfxMessageBox(IDS_NOVEDOBRODRUZSTVIDOKONCENO,MB_ICONINFORMATION);
  }
}

void CMainFrame::OnFileNatidobrodrustv()
{
  CString filter;
  filter.LoadString(IDS_ADVFILTER);
  CFileDialog fdlg(TRUE,_T(".ADV"),0,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,filter);
  if (fdlg.DoModal()==IDOK)
  {
	release_list(_adv);
	_adv=read_config(fdlg.GetPathName());
	if (_adv==0)
	{
	  AfxMessageBox(IDS_CHYBAPRICTENI);
	}
	else
	{
	  Pathname pth=fdlg.GetPathName();
	  SetTitle(pth);
	  _advPath=pth.GetDirectoryWithDrive();
	  _baseMapPath=_advPath+get_text_field(_adv,"CESTA_MAPY");
	  _advName=pth;
	}

  }
}

void CMainFrame::SetTitle(const _TCHAR *name)
{
  Pathname nm=name;
  CString title;
  title.Format(IDS_MAINFRAMETITLE,nm.GetTitle());
  SetWindowText(title);
}

void CMainFrame::OnEditoryKouzlatab()
{
  OpenEditor(_baseMapPath+"KOUZLA.TAB");
}

void CMainFrame::OnEditoryPostavytab()
{
  OpenEditor(_baseMapPath+"POSTAVY.TAB");

}

void CMainFrame::OnEditoryItemsscr()
{
  OpenEditor(_baseMapPath+"ITEMS.SCR");
}


void CMainFrame::OnEditoryItemspic()
{
  OpenEditor(_baseMapPath+"ITEMS.PIC");
}

void CMainFrame::OnEditoryWeaponsscr()
{
  OpenEditor(_baseMapPath+"WEAPONS.SCR");
}

static BOOL CALLBACK BroadcastMsgCallback(HWND wnd, LPARAM lParam)
{
  MSG *pMsg=(MSG *)lParam;
  LRESULT res=::SendMessage(wnd,pMsg->message,pMsg->wParam,pMsg->lParam);
  pMsg->time=res;
  return res==0;
}


LRESULT CMainFrame::BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
  MSG Msg;
  Msg.message=msg;
  Msg.wParam=wParam;
  Msg.lParam=lParam;
  Msg.time=0;
  EnumThreadWindows(GetCurrentThreadId(),BroadcastMsgCallback,(LPARAM)&Msg);
  return Msg.time;
}

void CMainFrame::SaveAll()
{
  BroadcastMessage(MSG_FORCESAVE,0,0);
}

void CMainFrame::OpenEditor(const char *name)
{
  if (BroadcastMessage(MSG_FINDANDPOPUP,0,(LPARAM)name)==0)
  {
	EditSkeldalFile(name);
  }
}

void CMainFrame::OnDestroy()
{
  SaveBarState("MainFrame");
	CFrameWnd::OnDestroy();

	BroadcastMessage(MSG_CLOSEEDITOR,0,0);
	release_list(_adv);
	_adv=0;


}

void CMainFrame::StartApp(const char *appname, CString cmdline, bool wait, const char *folder)
{
  SaveAll();
  STARTUPINFO nfo;
  memset(&nfo,0,sizeof(nfo));
  nfo.cb=sizeof(nfo);
  HANDLE pipe;
  if (wait)
  {
	HANDLE output;
	CreatePipe(&pipe,&output,0,0);
	DuplicateHandle(GetCurrentProcess(),output,GetCurrentProcess(),&output,0,TRUE,DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE);
	nfo.hStdOutput=output;
	nfo.hStdError=output;
	nfo.dwFlags|=STARTF_USESTDHANDLES;
	m_wndView.SetWindowText("");
  }
  Pathname app=appname;
  cmdline=_T("\"")+CString(appname)+_T("\" ")+cmdline;
  CString startdir;
  PROCESS_INFORMATION pi;
  if (folder==0)
	startdir=app.GetDirectoryWithDrive();
  else
	startdir=folder;
  if (startdir[startdir.GetLength()-1]=='\\') startdir.Delete(startdir.GetLength()-1);
  BOOL res=CreateProcess(appname,cmdline.LockBuffer(),0,0,TRUE,NORMAL_PRIORITY_CLASS|DETACHED_PROCESS,0,startdir,&nfo,&pi);
  cmdline.UnlockBuffer();
  if (res==FALSE)
  {
	CString ss;
	AfxFormatString1(ss,IDS_CANNOTEXECUTE,cmdline);
	AfxMessageBox(ss,MB_ICONSTOP);
  }
  else
  {
	CloseHandle(pi.hThread);
	if (wait)
	{
	  int rep=10;
	  bool end;
	  do
	  {
		DWORD datalen;
		PeekNamedPipe(pipe,0,0,0,&datalen,0);
		while (datalen)
		{
		  DWORD readed=0;
		  char buff[256];
		  if (ReadFile(pipe,&buff,255,&readed,0)==FALSE) break;
		  if (readed==0) break;
		  int cnt=m_wndView.GetWindowTextLength();
		  if (cnt>20000)
		  {
			m_wndView.SetSel(0,10000);
			m_wndView.ReplaceSel("");
			cnt=m_wndView.GetWindowTextLength();
		  }
		  buff[readed]=0;
		  kamenik2windows(buff,readed,buff);
		  m_wndView.SetSel(cnt,cnt);
		  m_wndView.ReplaceSel(buff);
		  PeekNamedPipe(pipe,0,0,0,&datalen,0);
		  rep=10;
		}
		UpdateWindow();
		end=WaitForSingleObject(pi.hProcess,200)==WAIT_TIMEOUT;
		rep--;
	  }
	  while (rep>0 || end);
	}
	CloseHandle(pi.hProcess);
  }
  if (wait)
  {
	CloseHandle(pipe);
	CloseHandle(nfo.hStdOutput);
  }
  SetForegroundWindow();
}

void CMainFrame::OnPekladaePelokouzla()
{
  CString cmdline=_T("\"")+_baseMapPath+_T("KOUZLA.TAB")+_T("\"");
  Pathname apppath=Pathname::GetExePath();
  CString appname=apppath.GetDirectoryWithDrive()+CString(_T("AdvManData\\CSPELLS.EXE"));
  StartApp(appname,cmdline,true,_baseMapPath);
}

void CMainFrame::OnPekladaePelodialogy()
{
  CString cmdline=_T("\"")+_baseMapPath+_T("DIALOGY.DLG")+_T("\"");
  Pathname apppath=Pathname::GetExePath();
  CString appname=apppath.GetDirectoryWithDrive()+CString(_T("AdvManData\\CDIALOGY.EXE"));
  StartApp(appname,cmdline,true,_baseMapPath);

}

void CMainFrame::OnPekladaePelopostavytab()
{
  CString cmdline=_T("\"")+_baseMapPath+_T("POSTAVY.TAB\" \"")+_baseMapPath+_T("POSTAVY.DAT")+_T("\"");
  Pathname apppath=Pathname::GetExePath();
  CString appname=apppath.GetDirectoryWithDrive()+CString(_T("AdvManData\\Lex_Lib.exe"));
  StartApp(appname,cmdline,true,_baseMapPath);

}

void CMainFrame::OnNstrojeMapedit()
{
  Pathname apppath=Pathname::GetExePath();
  apppath.SetFilename(_T("MapEdit.exe"));
  StartApp(apppath,CString(_T("\""))+_advName+_T("\""),false);
}

void CMainFrame::OnNstrojeTestujdobrodrustv()
{
  Pathname apppath=Pathname::GetExePath();
  apppath.SetFilename(_T("Skeldal.exe"));
  StartApp(apppath,CString(_T("\""))+_advName+_T("\""),false);
}

void CMainFrame::OnNstrojeTvrcepodlah()
{
  Pathname apppath=Pathname::GetExePath();
  apppath.SetFilename(_T("AdvManData\\Podlahar.exe"));
  StartApp(apppath,"",false,_advPath+get_text_field(_adv,"CESTA_GRAFIKA"));

}

void CMainFrame::OnNstrojeTvrcepaletpronestvry()
{
  Pathname apppath=Pathname::GetExePath();
  apppath.SetFilename(_T("AdvManData\\ColEdit.exe"));
  StartApp(apppath,"",false,_advPath+get_text_field(_adv,"CESTA_ENEMY"));
}


void CMainFrame::OnNstrojeTvrceikonpropedmty()
{
  Pathname apppath=Pathname::GetExePath();
  apppath.SetFilename(_T("AdvManData\\ItemIcons.exe"));
  StartApp(apppath,"",false,_advPath+get_text_field(_adv,"CESTA_ITEMY"));
}

void CMainFrame::OnEditorySouboradv()
{
  AfxMessageBox(IDS_EDITADVWARN,MB_ICONEXCLAMATION);
  OpenEditor(_advName);
}

void CMainFrame::OnFileZnovunast()
{
	release_list(_adv);
    _adv=read_config(_advName);
    _baseMapPath=_advPath+get_text_field(_adv,"CESTA_MAPY");
}

void CMainFrame::OnUpdateFileZnovunast(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(_adv!=0);
}

void CMainFrame::OnEditoryDialogy()
{
  DlgDialogy dlg;
  dlg._dlgpath.SetDirectory(_baseMapPath);
  dlg._dlgpath.SetFilename(_T("Dialogy.dlg"));
  dlg.DoModal();
}

void CMainFrame::OnUpdateEditory(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(_adv!=0);
}
