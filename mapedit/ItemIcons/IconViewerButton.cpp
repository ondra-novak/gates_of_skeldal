// IconViewerButton.cpp : implementation file
//

#include "stdafx.h"
#include "ItemIcons.h"
#include "IconViewerButton.h"
#include ".\iconviewerbutton.h"


// CIconViewerButton

IMPLEMENT_DYNAMIC(CIconViewerButton, CStatic)
CIconViewerButton::CIconViewerButton()
{
}

CIconViewerButton::~CIconViewerButton()
{
}


BEGIN_MESSAGE_MAP(CIconViewerButton, CStatic)
  ON_WM_PAINT()
  ON_WM_SETCURSOR()
END_MESSAGE_MAP()



// CIconViewerButton message handlers



void CIconViewerButton::OnPaint()
{
  unsigned short *data=reinterpret_cast<unsigned short *>(_data);
  if (data[0]!=45 && data[1]!=55 && data[2]!=8) 
  {
    __super::OnPaint();
    return;
  }


  BitMapInfo bmpinfo;
  ZeroMemory(&bmpinfo,sizeof(bmpinfo));
  bmpinfo.hdr.biSize=sizeof(bmpinfo.hdr);
  bmpinfo.hdr.biBitCount=8;
  bmpinfo.hdr.biCompression=BI_RGB;
  bmpinfo.hdr.biHeight=55;
  bmpinfo.hdr.biPlanes=1;
  bmpinfo.hdr.biWidth=45; 
  for (int i=0;i<256;i++) 
  {
    RGBQUAD &col=bmpinfo.palette[i];
    col.rgbBlue=(data[i+3] & 0x1F)<<3;
    col.rgbGreen=(data[i+3] & 0x3E0)>>2;
    col.rgbRed=(data[i+3] & 0x7C00)>>7;
    col.rgbReserved=0;
  }

  bmpinfo.palette[0].rgbBlue=80;
  bmpinfo.palette[0].rgbGreen=70;
  bmpinfo.palette[0].rgbRed=60;

  char bitmap[55][48];
  char *p=reinterpret_cast<char *>(data+3+256);
  for (int i=0;i<55;i++,p+=45) memcpy(bitmap[55-i-1],p,45);

  CPaintDC dc(this);
  CRect rc;
  GetClientRect(&rc);
  dc.SetStretchBltMode(HALFTONE);
  StretchDIBits(dc,0,0,rc.right,rc.bottom,0,0,45,55,bitmap,reinterpret_cast<BITMAPINFO *>(&bmpinfo),DIB_RGB_COLORS,SRCCOPY);
}

BOOL CIconViewerButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  if (pWnd==this)
  {
    HCURSOR h=LoadCursor(0,IDC_HAND);
    ::SetCursor(h);
    return TRUE;
  }
  else
    return __super::OnSetCursor(pWnd,nHitTest,message);

}
