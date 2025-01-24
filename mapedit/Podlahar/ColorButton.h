#pragma once


// CColorButton

class CColorButton : public CButton
{
	DECLARE_DYNAMIC(CColorButton)
    COLORREF _color;

public:
	CColorButton();
	virtual ~CColorButton();

    void SetColor(COLORREF color, BOOL redraw=1);
    COLORREF GetColor() const {return _color;}


protected:
	DECLARE_MESSAGE_MAP()

public:
  virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
};


