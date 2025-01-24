/*
   PCSPEAKER TESTING

 Tento program testuje knihovnu PCSPEAK.ASM.
 Program alokuje 64Kb v dolni casti pameti. Pote zacne tento blok prehravat
 stale dokola rychlosti 16000 Hz. Prehrava se nahodny obsah, takze by mel byt
 slyset nejakej sum nebo neco podobneho.

*/

#include <stdio.h>
#include <conio.h>
#include <pcspeak.h>
#include <i86.h>

UWORD seg;    //Segment bufferu
UWORD sel;    //Selector na buffer

  //Funkce pro alokaci v dolni casti pameti.
  // Alloc dos memory
void dosalloc (unsigned para,unsigned short *rmseg,unsigned short *select)
{
union REGS regs;

regs.w.ax = 0x100;
regs.w.bx = para;
int386 (0x31,&regs,&regs);
*rmseg = regs.w.ax;
*select = regs.w.dx;
}

  //Funkce pro uvolneni v dolni casti pameti.
	// Free dos memory
void dosfree (unsigned short select)
{
union REGS regs;

regs.w.ax = 0x101;
regs.w.dx = select;
int386 (0x31,&regs,&regs);
}

void alloc_buffer() //nejprve se zaalokuje buffer
  {
  dosalloc(65536/16,&seg,&sel);
  }

void dealloc_buffer() //dealokace
  {
  dosfree(sel);
  }

void speaker_test()
  {
  puts("Init...");
  rm_proc_set(seg,sel,PORT_SPK,SPK_MODE);//Nastav rm_proc
  load_rm_proc();               //nahraj rm_proc do dolni pameti
  pc_speak_enable();            //PCSPEAKER do modu DAC (pokud se hraje na spk)
  pc_speak_run(19000,18);       //Zacni prehravat buffer (18Hz emulacni frekvence)
  puts("Running...");
  getche();                     //cekej na klavesu
  puts("Stopping...");
  pc_speak_stop();              //Zastav prehravani
  pc_speak_disable();           //PCSPEAKER vrat do normalniho rezimu
  purge_rm_proc();              //vymaz rm_proc z dolni pameti
  puts("Ending...");
  }

void main()
  {
  alloc_buffer();               //Alokuj buffer
  speaker_test();               //Test
  dealloc_buffer();             //Dealokuj buffer
  }
