#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include "decoder.c"
#include "gif.h"

int get_byte ()
{
        int x;
        x=gif_file[gif_pozice];
        gif_pozice++;
        if (gif_pozice==LENGHTGIF) return (READ_ERROR);     else return (x);
}

int out_line (char *pixels, int linelen)
{
		memmove ((gif_picture+linelen*RaW), pixels, linelen);
        RaW++;
        if (RaW<=480) return (RaW);     else return (0);
}

int gif_to_buffer (char *filename)
{
	FILE *picture;
	int x;
	char mezi[4];

//-------------------------------------------------------------------------
//	Nacteni GIFu
//-------------------------------------------------------------------------
        picture=fopen(filename,"rb");

// Read size and alloc memory
		fseek (picture, 6, SEEK_SET);
		fread (mezi, 4, 1, picture);
		gif_size=(mezi[0]+mezi[1]*256)*(mezi[2]+mezi[3]*256);
		x_0=mezi[0];
		x_1=mezi[1];
		y_0=mezi[2];
		y_1=mezi[3];
		gif_picture=(char*)malloc( (gif_size*sizeof(gif_picture)) );

// Read and prepare palette
        fseek (picture, 0x0d, SEEK_SET);
        fread (gif_palette, 768, 1, picture);
		for (x=0; x<=768; x++) gif_palette[x]>>=3;

// Read coded picture
        fseek (picture, 0x317, SEEK_SET);
        fread (gif_file, LENGHTGIF, 1, picture);

// Decode picture
        RaW=0;
        decoder (640);
        fclose (picture);
//-------------------------------------------------------------------------

return (0);
}

