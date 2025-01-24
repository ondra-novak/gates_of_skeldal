// PaletteEditor.cpp : implementation file
//

#include "stdafx.h"
#include "ColEdit.h"
#include "PaletteEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPaletteEditor

CPaletteEditor::CPaletteEditor()
{
  memset(_selection,0,sizeof(_selection));
}

CPaletteEditor::~CPaletteEditor()
{
}


BEGIN_MESSAGE_MAP(CPaletteEditor, CStatic)
	//{{AFX_MSG_MAP(CPaletteEditor)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPaletteEditor message handlers

void CPaletteEditor::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rc;
	GetClientRect(&rc);
	char *p=_palette;

	for (int i=0;i<16;i++)
	  for (int j=0;j<16;j++)
	  {
		int x1=rc.right*j/16;
		int y1=rc.bottom*i/16;
		int x2=rc.right*(j+1)/16-1;
		int y2=rc.bottom*(i+1)/16-1;

		COLORREF ref=RGB(p[0],p[1],p[2]);
		bool white=(p[0]*0.3+p[1]*0.5+p[2]*0.2)<128;
		p+=3;
		dc.FillSolidRect(x1,y1,x2-x1,y2-y1,ref);
		dc.SelectStockObject(white?WHITE_PEN:BLACK_PEN);
		if (!_selection[i*16+j])
		{
		  dc.SelectStockObject(HOLLOW_BRUSH);
		  dc.Rectangle(x1,y1,x2,y2);
		  dc.MoveTo(x1,y1);dc.LineTo(x2,y2);
		  dc.MoveTo(x1,y2);dc.LineTo(x2,y1);
		}
	  }
}
	// Do not call CStatic::OnPaint() for painting message}


void CPaletteEditor::OnLButtonDown(UINT nFlags, CPoint point) 
{	
	int pt=IndexFromPoint(point);
	if (!(nFlags & MK_CONTROL))	
	  memset(_selection,0,sizeof(_selection));
	if (pt>=0 && pt<256)
	  _selection[pt]=!_selection[pt];
	_drawsel=_selection[pt];
	Invalidate();
	SetCapture();
}
	

void CPaletteEditor::OnMouseMove(UINT nFlags, CPoint point) 
{
  if (GetCapture()==this)
  {
	int pt=IndexFromPoint(point);
	if (pt>=0 && pt<256)
	  _selection[pt]=_drawsel;
	Invalidate();
  }
}

int CPaletteEditor::IndexFromPoint(CPoint pt)
{
	CRect rc;
	GetClientRect(&rc);
	int x=pt.x*16/rc.right;
	int y=pt.y*16/rc.bottom;
	int ret=y*16+x;
	return ret;
}

void CPaletteEditor::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if (GetCapture()==this) 
  {
	ReleaseCapture();
	GetParent()->SendNotifyMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),BN_CLICKED),(LPARAM)GetSafeHwnd());
  }
	
}
