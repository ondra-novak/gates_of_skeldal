#pragma once


// CIconViewerButton

  struct BitMapInfo
  {
    BITMAPINFOHEADER hdr;
    RGBQUAD palette[256];
  };


class CIconViewerButton : public CStatic
{
	DECLARE_DYNAMIC(CIconViewerButton)

public:
	CIconViewerButton();
	virtual ~CIconViewerButton();

    void *_data;

protected:
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnPaint();
  afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};


