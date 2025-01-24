/************************************/
/*** Hlavickovt soubor k DOSMEM.H ***/
/************************************/


typedef enum
{ DOS_MEMORY,
  NEW,
} MEMORY_ITEMS;

typedef struct
{ int EDI;
  int ESI;
  int EBP;
  int reserved;
  int EBX;
  int EDX;
  int ECX;
  int EAX;
  short int Flags;
  short int ES;
  short int DS;
  short int GS;
  short int IP;
  short int CS;
  short int SP;
  short int SS;
  } DPMIREGS;

typedef struct
{ void *ptr;
  int  size;
  int  selector;  //smysl jen u DOS_MEMORY
  } MEMREC;

#define D32RealSeg(P) ((((unsigned int)(P))>>4)&0xFFFF)
#define D32RealOff(P) (((unsigned int)(P))&0xF)

int Selector;
DPMIREGS dpmiregs;

void *mem_alloc(int size);
void mem_free(void *ptr); 
void WtNs386(int IntNum, DPMIREGS *dpmiregs); 


