//
//		Hlavickovy soubor ke knihovne FLC.C slouzici k dekompresi FLC
//

#ifndef _FLC_H
#define _FLC_H

typedef struct FLCHEADER {
	unsigned int size;		// delka souboru vcetne hlavicky
	unsigned short idflc;	// ID FLC=0AF12h
	unsigned short frames;	// pocet frejmu
	unsigned short width;	// sirka vsech obrazku
	unsigned short height;	// vyska vsech obrazku
	unsigned short color;	// hloubka barvy (bpp)
	unsigned short flag1;
	unsigned int speed;		// rychlost prehravani (v 1/1000 s)
	unsigned short reserv1;	// rezervovany
	unsigned int date1;		// datum a cas vytvoreni
	unsigned int serial;	// seriove cislo programu
	unsigned int date2;		// datum a cas posledni zmeny
	unsigned short XA;
	unsigned short YA;
	char reserv2 [42];		// rezervovano
	unsigned int offset1;	// offset prvniho frame
	unsigned int offset2;	// offset druheho frame
	char reserv3 [40];		// rezervovano
	} flcheader;

typedef struct FRAMEHEADER {
	unsigned int size;		// velikost frame v bytech
	unsigned short sign;	// znacka frame OFAF1h
	unsigned short actions;	// pocet akci v tomto frame
	char reserv [8];		// rezervovano
	} frameheader;

typedef struct ACTIONHEADER {
	unsigned int size;		// velikost akce v bytech
	unsigned short code;	// kod akce
		// 04h - predani casti nebo cele palety
		// 07h - predavani zmenenych casti obrazu
		// 0Dh - vymaze obrazovku
		// 0Fh - cela obrazovka v RLE
		// 10h - nekomprimovana kopie cele obrazovky
	// nasleduji data akce
	} actionheader;

char frame_buffer [307200];
char *flc_buffer;

void Get_first_frame (void);
void Get_next_frame (void);
void Decompress_frame (void);

#endif
