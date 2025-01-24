#include <stdio.h>
#include <string.h>
#include "gif.h"

unsigned short new_palette [256];

void main (int argc, char *argv[])
{
	FILE *xed;
	char gif[13];
	int x,y;

	memmove (gif, argv[1], 13);
	xed=fopen("new.xed","wb");

	gif_to_buffer (gif);

	y=0;	
	for (x=0; x<=767; x+=3)
		{
		new_palette[y]=gif_palette[(x+2)];
		new_palette[y]|=gif_palette[(x+1)]<<5; //*32;
		new_palette[y]|=gif_palette[x]<<10; //*1024;
		y++;
		}

	fputc (x_0,xed);
	fputc (x_1,xed);
	fputc (y_0,xed);
	fputc (y_1,xed);
	fputc (8,xed);
	fputc (0,xed);

	fwrite (new_palette, 256, 2, xed);
	fwrite (gif_picture, gif_size, 1, xed);

	fclose (xed);
}

