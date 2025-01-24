// EditPaletteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ColEdit.h"
#include "EditPaletteDlg.h"
#include <math.h>
#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditPaletteDlg dialog


CEditPaletteDlg::CEditPaletteDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditPaletteDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditPaletteDlg)
	//}}AFX_DATA_INIT
}


void CEditPaletteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditPaletteDlg)
	DDX_Control(pDX, IDC_SITOST, wSitost);
	DDX_Control(pDX, IDC_JAS, wJas);
	DDX_Control(pDX, IDC_BARVA, wBarva);
	DDX_Control(pDX, IDC_PALETA, wPaleta);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditPaletteDlg, CDialog)
	//{{AFX_MSG_MAP(CEditPaletteDlg)
	ON_BN_CLICKED(IDC_PALETA, OnPaleta)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_UNDO, OnUndo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditPaletteDlg message handlers

/*
RGB to HSV & HSV to RGB

The Hue/Saturation/Value model was created by A. R. Smith in 1978. It is based on such intuitive color characteristics as tint, shade and tone (or family, purety and intensity). The coordinate system is cylindrical, and the colors are defined inside a hexcone. The hue value H runs from 0 to 360o. The saturation S is the degree of strength or purity and is from 0 to 1. Purity is how much white is added to the color, so S=1 makes the purest color (no white). Brightness V also ranges from 0 to 1, where 0 is the black.

There is no transformation matrix for RGB/HSV conversion, but the algorithm follows:
*/
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
static void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
	float min, max, delta;
	min = __min( r, __min(g, b ));
	max = __max( r, __max(g, b ));
	*v = max;				// v
	delta = max - min;
	if( max != 0 )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
}
static void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;
	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
	h /= 60;			// sector 0 to 5
	i = (int)floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
        case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}


BOOL CEditPaletteDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	memcpy(_paletteUndo,_palette,768);
	wPaleta.SetPalette(_palette);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPaletteDlg::CalculateAvgHSV()
{
  float h,s,v,sh=0,ss=0,sv=0;
  int cnt=0;
  for (int i=0;i<256;i++) if (wPaleta.IsSelected(i))
  {
	RGBtoHSV(wPaleta.GetR(i)/255.0f,wPaleta.GetG(i)/255.0f,wPaleta.GetB(i)/255.0f,&h,&s,&v);
    if (!_finite(h)) h=0;
	sh+=h;
	ss+=s;
	sv+=v;
	cnt++;
  }
  if (cnt==0) return;
  sh/=cnt;
  ss/=cnt;
  sv/=cnt;
  wBarva.SetRange(0,359);
  wBarva.SetPos((int)sh);
  wSitost.SetRange(0,100);
  wSitost.SetPos((int)(ss*100));
  wJas.SetRange(0,100);
  wJas.SetPos((int)(sv*100));
  _curh=sh,_curv=sv,_curs=ss;
  memcpy(_paletteScrl,_palette,768);
  
}

void CEditPaletteDlg::OnPaleta() 
{
  CalculateAvgHSV();    	
}

void CEditPaletteDlg::OnCancel() 
{
	memcpy(_palette,_paletteUndo,768);
    wPaleta.Invalidate(FALSE);
  _imgView->SetPalette(_palette);
   _imgView->Invalidate(FALSE);
	CDialog::OnCancel();
}

void CEditPaletteDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  float h,s,v;
  float difh=wBarva.GetPos()-_curh;
  float difs=(wSitost.GetPos()/100.0f)/_curs;
  float difv=(wJas.GetPos()/100.0f)/_curv;
  for (int i=0;i<256;i++) if (wPaleta.IsSelected(i))
  {
	RGBtoHSV(_paletteScrl[i*3]/255.0f,_paletteScrl[i*3+1]/255.0f,_paletteScrl[i*3+2]/255.0f,&h,&s,&v);
	if (!_finite(h)) h=0;
	h+=difh;
	while (h<0) {h+=360.0f;}
	while (h>=360.0f) {h-=360.0f;}
	if (s>0.001)
	  s*=difs;
	else
	  s=wSitost.GetPos()/100.0f;
	if (v>0.001)
	  v*=difv;
	else
	  v=wJas.GetPos()/100.0f;
	if (s<0) s=0;
	if (s>1.0f) s=1.0f;
	if (v<0) v=0;
	if (v>1.0f) v=1.0f;
	float r,g,b;
	HSVtoRGB(&r,&g,&b,h,s,v);
	_palette[i*3]=(int)(r*255.0f);
	_palette[i*3+1]=(int)(g*255.0f);
	_palette[i*3+2]=(int)(b*255.0f);
  }
  wPaleta.Invalidate(FALSE);
  wPaleta.UpdateWindow();
  _imgView->SetPalette(_palette);
  _imgView->Invalidate(FALSE);
  _imgView->UpdateWindow();
}

void CEditPaletteDlg::OnUndo() 
{
  memcpy(_palette,_paletteScrl,768);
  wPaleta.Invalidate(FALSE);
  _imgView->SetPalette(_palette);
  _imgView->Invalidate(FALSE);
}
