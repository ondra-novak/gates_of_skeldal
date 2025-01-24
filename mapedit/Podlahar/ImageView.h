#pragma once


// CImageView

class CImageView : public CStatic
{
	DECLARE_DYNAMIC(CImageView)
    char *_buffer;
    struct BMI
    {
      BITMAPINFOHEADER _bmh;
      RGBQUAD _palette[256];
    } _bmi;
public:
	CImageView();
	virtual ~CImageView();

protected:
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnPaint();
  void UpdateBuffer(char *buffer);
  void SetPalette(char *palette);
};


