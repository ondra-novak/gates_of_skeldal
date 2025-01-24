/********************************/
/* Prace se SVGA VESA adapterem */
/********************************/

#ifndef __VESA_H
#define __VESA_H


typedef struct
{ char VESASignature[4];
  short VESAversion;
  unsigned OEMStringPtr;
  char capabilities[4];
  short int *videomodeptr;
  short int TotalMemory;
  char dummy[236];
  } VESA_INFO_BLOCK;

typedef struct
{ short unsigned ModeAttributes;
  char WinAAttributes;
  char WinBAttributes;
  short unsigned WinGranularity;
  short unsigned WinSize;
  short unsigned WinASegment;
  short unsigned WinBSegment;
  short unsigned WinFuncPtroff;
  short unsigned WinFuncPtrseg;
  short unsigned BytesPerScanLine;

  short unsigned XResolution;
  short unsigned YResolution;
  char Xcharsize;
  char Ycharsize;
  char NumberOfPlanes;
  char BitsPerPixel;
  char NumberOfBanks;
  char MemoryModel;
  char BankSize;
  char NumberOfImagePages;
  char Reserved;

  char RedMaskSize;
  char RedFieldPosition;
  char GreenMaskSize;
  char GreenFieldPosition;
  char BlueMaskSize;
  char BlueFieldPosition;
  char RsvdMaskSize;
  char DirectColorModeInfo;
  char Dummy[216];
  } VESA_MODE_INFO_BLOCK;


VESA_MODE_INFO_BLOCK VesaModeInfoBlock;
VESA_INFO_BLOCK VesaInfoBlock;

extern unsigned char _VGAPage=0;
extern unsigned char _VGAGran=0;

void Set_VESA_mode (int mode);
void Show_screen (char *display);
void Get_VESA_info (void);
void Get_mode_info (int mode);

void Put_image (int x, int y, int xlen, int ylen, char *image);
void Get_image (int x, int y, int xlen, int ylen, char *image);

#endif

