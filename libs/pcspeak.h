#define SPK_MODE 1
#define DAC_MODE 0

#define PORT_LPT1 0x378
#define PORT_LPT2 0x278
#define PORT_SPK  0x42

#define UWORD unsigned short

void rm_proc_set(UWORD bufseg,UWORD bufsel,UWORD port,UWORD mode);
#pragma aux rm_proc_set parm [eax][edx][ecx][ebx] modify [esi edi]

char load_rm_proc(void);
#pragma aux load_rm_proc modify [eax ebx ecx edx esi edi] value [al]

char purge_rm_proc(void);
#pragma aux purge_rm_proc modify [edx eax] value [al]

void pc_speak_run(int32_t s_freq,int32_t sim_freq);
#pragma aux pc_speak_run parm[eax][edx] modify [ecx ebx]

void pc_speak_stop(void);
#pragma aux pc_speak_stop modify [eax ebx ecx edx]

void pc_speak_enable(void);
#pragma aux pc_speak_enable modify [eax]

void pc_speak_disable(void);
#pragma aux pc_speak_enable modify [eax]

int32_t pc_speak_position(void);
#pragma aux pc_speak_position modify[eax ebx] value [eax]

void pc_speak_set_proc(int32_t *c);
#pragma aux pc_speak_set_proc parm [edi]

/* Zde jsou nejake komentare */

/*

void rm_proc_set;
 Vstup: bufseg - segmen bufferu v konvenci realneho rezimu
        bufsel - selektor tohoto segmentu (protect rezim)
        port - cislo portu - je mozne vyuzit konstant PORT_LPT? nebo PORT_SPK
        mode - zde vyuzij konstant SPK_MODE nebo DAC_MODE podle zarizeni.
 Vystup: -
 Komentar: Podprogram nastavuje parametry rm procedury pro spravne prehravani
           Je nutne provest pred load_rm_proc.
           MODE musi korespondovat s PORTem, je-li PORT=PORT_SPK, pak
              MODE=SPK_MODE
           Nemusim doufam rikat ze bufseg a bufsel musi popisovat stejny
              segment.

 -------------------------------------------------------------------

void load_rm_proc;
 Vstup: -
 Vystup:nenulova hodnota v pripade nedostatku dolni pameti.
 Komentar: Funkce nahraje do realne pameti (do 1MB) rm_proc pro prehravani
           Rm_proc pak zajisti spravnou obsluhu preruseni v realnem rezimu

 -------------------------------------------------------------------

void purge_rm_proc;
 vstup: -
 Vystup:nenulova hodnota oznamuje ze se nepodarilo rm_proc uvolnit z pameti.
 Komentar: Funkce uvolni pamet, ktera byla alokovana funkci load_rm_proc.
           Tim dojde ke zniceni rm_proc.

 -------------------------------------------------------------------

void pc_speak_run;
 vstup: s_freq - samplovaci frekvence v Hz (treba 22050)
        sim_freq - frekvence simulace INT 8. Bezne 18Hz. (frekvence je priblizna)
 vystup:zacne se prehravat
 Komentar: Funkce alokuje preruseni INT 8 a spusti ho rychlosti danou S_FREQ.
           pm_proc se navic simuluje puvodni INT 8 frekvenci sim_freq.
           simulace INT 8 neprobiha v realnem rezimu, pouze v chranennem.
           (pozor na DOS). Behem simulace je preruseni povolene (chranenne
           zavorou v pm_proc). Pokud chces pouzivat INT 8, nesmis zakazovat
           preruseni behem jeho zpracovavani, zabranis tak casovym ztratam
           na INT 8.

 -------------------------------------------------------------------

void pc_speak_stop;
 vstup: -
 vystup:-
 Komentar: Zastavi prehravani. Hodiny neobnovuje - spravny cas si
           musis nastavit sam (nejlepe z CMOS).


 -------------------------------------------------------------------

void pc_speak_enable;
void pc_speak_disable;

 enable - zapina PC SPEAKER v rezimu 6-bitoveho DAC.
 disable - vraci nastaveni PC SPEAKERu zpet do puvodniho rezimu.


 -------------------------------------------------------------------

int32_t pc_speak_position(void);

 Vstup: -
 Vystup: int32_t offset do bufferu. Vraci prave preravanou pozici
 Komentar: sice vraci int32_t, ale offset je v rozsahu <0-64Kb>. Vzhledem k
           casovym ztratam muze vracena pozice byt starsi uz v dobe
           predavani vysledku. Presnost zavisi na rychlosti pocitace.

 -------------------------------------------------------------------

void pc_speak_set_proc;
 Vstup: Ukazatel na ukazatel na void (ackoliv je tam int32_t *)
 Vystup:-
 Komentar: Procedura modifikuje promennou kam ukazuje parametr tak aby
           obsahovala adresu na proceduru pc_speak_position.
           Volanim teto adresy lze pak docilit stejne funkce jako
           volanim primeho pc_speak_position.
           Oba zpusoby jsou rovnocenne.

 -------------------------------------------------------------------


Doporuceny postup:

  rm_proc_set(...);//Nastav rm_proc
  load_rm_proc();               //nahraj rm_proc do dolni pameti
  pc_speak_enable();            //PCSPEAKER do modu DAC (pokud se hraje na spk)
  pc_speak_run(...);            //Zacni prehravat buffer
  ......
  pc_speak_stop();              //Zastav prehravani
  pc_speak_disable();           //PCSPEAKER vrat do normalniho rezimu
  purge_rm_proc();              //vymaz rm_proc z dolni pameti

  */
