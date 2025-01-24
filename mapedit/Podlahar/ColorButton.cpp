// ColorButton.cpp : implementation file
//

#include "stdafx.h"
#include "ColorButton.h"
#include ".\colorbutton.h"


// CColorButton

IMPLEMENT_DYNAMIC(CColorButton, CButton)
CColorButton::CColorButton()
{
}

CColorButton::~CColorButton()
{
}


BEGIN_MESSAGE_MAP(CColorButton, CButton)
END_MESSAGE_MAP()



// CColorButton message handlers


void CColorButton::DrawItem(LPDRAWITEMSTRUCT dis)
  {
  CRect rcitem(dis->rcItem);
  CDC dc;
  dc.Attach(dis->hDC);
  dc.DrawFrameControl(&dis->rcItem,DFC_BUTTON,DFCS_BUTTONPUSH|(dis->itemState & 1?DFCS_PUSHED:0));
  rcitem-=CRect(2,2,2,2);
  if (dis->itemState & ODS_FOCUS) dc.DrawFocusRect(&rcitem);
  rcitem-=CRect(2,2,2,2);
  dc.FillSolidRect(&rcitem,_color);  
  dc.Detach();
  }

void CColorButton ::SetColor(COLORREF color, BOOL redraw)
  {
  _color=color;
  Invalidate(FALSE);
  }
