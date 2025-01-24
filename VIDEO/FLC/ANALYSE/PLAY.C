#include <stdio.h>
#include <conio.h>
#include <graph.h>
#include "flc.h"

flcheader h_flc;
frameheader h_frame;
actionheader h_action;


void main (int argc, char *argv[])
{
	FILE *flc;
	long pozice;
	int frames, actions, x, y;

	flc = fopen (argv[1], "rb");
	fread (&h_flc, sizeof(flcheader), 1, flc);
	frames=h_flc.frames;

	printf ("\nHlavicka FLC souboru %s:\n", argv[1]);
	printf ("Delka celeho souboru: %d\n", h_flc.size);
	printf ("Pocet frame: %d\n", h_flc.frames);
	printf ("Velikost filmu: %dx%d\n", h_flc.width, h_flc.height);
	printf ("Hloubka barvy: %d\n", h_flc.color);
	printf ("Rychlost prehravani: %d fps\n", 1000/h_flc.speed);
	printf ("Offset prvniho frame: %d\n", h_flc.offset1);

	fseek (flc, h_flc.offset1, SEEK_SET);
	for (x=0; x<=(frames-1); x++)
	{
		fread (&h_frame, sizeof(frameheader), 1, flc);
		actions=h_frame.actions;
		printf ("\nHlavicka %d framu:\n", x+1);
		printf ("Velikost framu: %d\n", h_frame.size);
		printf ("Pocet akci ve framu: %d\n", h_frame.actions);

		for (y=0; y<=(actions-1); y++)
		{
			pozice = ftell (flc);
			fread (&h_action, sizeof(actionheader), 1, flc);
			fseek (flc, (pozice+h_action.size), SEEK_SET);
			printf ("\nHlavicka %d akce:\n", y+1);
			printf ("Velikost dat pro tuto akci: %d\n", h_action.size);
			printf ("Kod akce: %x\n", h_action.code);
		}
	getch ();
	_clearscreen (_GCLEARSCREEN);
	}

	fclose (flc);
}

		
