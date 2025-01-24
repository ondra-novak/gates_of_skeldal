//
//              Knihovna pro dekompresi FLC souboru
//

#include <stdio.h>
#include <conio.h>
#include <malloc.h>
#include <string.h>
#include <i86.h>
#include "flc.h"

FILE *flc;
flcheader h_flc;
frameheader h_frame;
actionheader h_action;
char flc_paleta[256][3];
char frame_buffer [307205];
char *flc_buffer;
char test=1;

void Open_FLC (char *filename)
{
	flc = fopen (filename, "rb");
	fread (&h_flc, sizeof(flcheader), 1, flc);
}

void Close_FLC (void)
{
	fclose (flc);
	free (flc_buffer);
}

void Get_first_frame (void)
{
	fseek (flc, h_flc.offset1, SEEK_SET);
	fread (&h_frame, sizeof(frameheader), 1, flc);
	flc_buffer = (char*)malloc((h_frame.size-sizeof(frameheader)));
	fread (flc_buffer, (h_frame.size-sizeof(frameheader)), 1, flc);
}

void Get_next_frame (void)
{
	free (flc_buffer);
	fread (&h_frame, sizeof(frameheader), 1, flc);
	flc_buffer = (char*)malloc((h_frame.size-sizeof(frameheader)));
	fread (flc_buffer, (h_frame.size-sizeof(frameheader)), 1, flc);
}

void Decompress_frame (void)
{
	unsigned short x,y,z,w;
	unsigned short changes, nfo_word, packets, offset, row;
  int frame_pos=0,tst;
	unsigned int scr_pos;
	char c1, c2, c3, a;
	char hi, lo;

  packets=0;
  for (z=1; z<=h_frame.actions; z++)
  {
		memmove ( &h_action, (flc_buffer+frame_pos), sizeof(actionheader) );
		switch (h_action.code)
		{
        case  16:
           frame_pos+=6;
           memcpy(frame_buffer,flc_buffer+
              frame_pos,h_flc.width*h_flc.height);
           frame_pos+=h_flc.width*h_flc.height;
           break;
			case  4:
              {
              short bc;
              int barv,poc;
              int fp;

              fp=frame_pos+6;
              bc=*(short *)(flc_buffer+fp);fp+=2;
              barv=0;
              while(bc--)
                 {
                 barv+=(flc_buffer[fp++]);
                 poc=(char)(flc_buffer[fp++]-1)+1;
                 memcpy(&flc_paleta[barv][0],flc_buffer+fp,poc*3);
                 fp+=poc*3;barv+=poc;
                 }
              frame_pos+=h_action.size;
              }
				break;
			case  7:
        case  12:
				frame_pos+=6;
				lo=flc_buffer[frame_pos]; frame_pos++;
           hi=flc_buffer[frame_pos++];
				changes=hi*256+lo;
           row=0;tst=frame_pos+h_action.size;

           for (y=0; frame_pos<tst ; y++)         // pocet menenych radek
				{
					lo=flc_buffer[frame_pos]; frame_pos++;
              hi=flc_buffer[frame_pos++];
					nfo_word=hi*256+lo;

              scr_pos=row*h_flc.width/*640*/;

              if (nfo_word>=0x8000)               // preskakovane radky
					{
              nfo_word=(short)(-nfo_word); //0xFFFF-nfo_word+1;
					row+=nfo_word;
					}

					else
					{
              for (z=0; z<nfo_word; z++)         // pocet menenych bloku
					{
						x=1; a=0;

						offset = flc_buffer[frame_pos];	 // rel. offset bloku
						frame_pos++;
						scr_pos+=offset;

//						while (!a)					// zmena bloku
//						{
							c1 = flc_buffer[frame_pos];
							frame_pos++;

                    if (frame_pos>=tst) break;
							if (c1>128)
							{
								c1=0xFF-c1+1;
								c2=flc_buffer[frame_pos];
								frame_pos++;
								c3=flc_buffer[frame_pos];
								frame_pos++;

								for (w=1; w<=c1; w++)
								{	frame_buffer[scr_pos]=c2;
									scr_pos++;
									frame_buffer[scr_pos]=c3;
									scr_pos++; }
							}
							else
							{
//								c3=0xFF-c3+1;
								for (w=1; w<=c1; w++)
								{	frame_buffer[scr_pos]=flc_buffer[frame_pos];
									frame_pos++;
									scr_pos++;
									frame_buffer[scr_pos]=flc_buffer[frame_pos];
									frame_pos++;
									scr_pos++; }
							}

//							if (x>=640) a=1;
//						}

					}
					row++;
					}
				}

//				frame_pos+=h_action.size;
				break;
			case 15:
				frame_pos+=6;
				for (y=0; y<=(h_flc.height-1); y++)
				{
					frame_pos++; x=1; //a=0;
					scr_pos=y*h_flc.width;

					while (x<=h_flc.width)
					{
						c1 = flc_buffer[frame_pos];
						frame_pos++;

						if (c1<128)
						{
							c2=flc_buffer[frame_pos];
							frame_pos++;
							for (w=1; w<=c1; w++)
							{	frame_buffer[scr_pos]=c2;
								scr_pos++;
								x++; }
						}
						else
						{
							c1=0xFF-c1+1;
							for (w=1; w<=c1; w++)
							{	frame_buffer[scr_pos]=flc_buffer[frame_pos];
								scr_pos++;
								frame_pos++;
								x++; }
						}
					}
				}
				break;
			default: frame_pos+=h_action.size;
				break;
		};

	}

}
