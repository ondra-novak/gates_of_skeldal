#ifndef _GIF__H
#define _GIF__H

char gif_file [512000];
char *gif_picture;
char gif_palette [768];
int x_0, x_1, y_0, y_1, gif_size;
long int gif_pozice=0;
int RaW=0;

int gif_to_buffer (char *filename);

#define LENGHTGIF 512000

#endif

