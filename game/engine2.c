#include <platform/platform.h>
#include <libs/types.h>
#include "engine1.h"
#include <libs/bgraph.h>

#include <string.h>

typedef ZOOMINFO tzoom;

extern ZOOMINFO zoom;
extern word *screen;

void sikma_zleva(void)
{

    int32_t scr_linelen2 = GetScreenPitch();
	word *scr = (word *)zoom.startptr;
	const word *palette = (word *)zoom.palette;
	word cy = zoom.ycount;
	const unsigned char *pixmap = zoom.texture;
	const short *ytable = zoom.ytable;
	while (cy) {
		const int32_t *xtable = zoom.xtable;
		word cx = zoom.xmax;
		word *scr_iter = scr;
		const unsigned char *pixmap_iter = pixmap;
		while (cx > 0) {
			unsigned char pb = *pixmap_iter++;
			pixmap_iter+=*xtable;
			xtable++;
			if (pb == 1) break;
			if (pb != 0) *scr_iter = palette[pb];
			scr_iter++;
			cx--;
		}
		pixmap+=zoom.texture_line*(*ytable);
		ytable++;
		scr-=scr_linelen2;
		cy--;
	}



	/*


	__asm
	{
	mov     edi,zoom       ;nacti ukazatel do obrazovky
	mov     ebx,[zoom]tzoom.palette ;ukazatel na paletu
	mov     cx,short ptr [zoom]tzoom.ycount ;velikost textury na y
	shl     ecx,16  ;vloz do horni pulky ecx
	mov     esi,[zoom]tzoom.texture ;nacti ukazatel na texturu
	skzl3:  mov     edx,[zoom]tzoom.xtable ;nacti ukazetel na zvetsovaci tabulku x
	push    esi ;uchovej esi
	push    edi ;uchovej edi
	mov     cx,[zoom]tzoom.xmax
	skzl1:  xor     eax,eax ;vynuluj eax pro spravny vypocet
	lodsb       ;nacti bod
	add     esi,[edx] ;posun se od nekolik pozic v texture podle hodnoty v tabulce x
	add     edx,4 ;posun se v tabulce x o dalsi polozku
	or      al,al ;test bodu na nulu
	jz      skz1  ;preskoc transparetni barvu
	cmp     al,1  ;test bodu na jedna
	jz      skz2  ;ukonci kresleni linky pokud narazi na 1
	mov     ax,[eax*2+ebx] ;konverze barvy podle palety
	mov     [edi],ax ;nakresli bod na obrazovce
	skz1:   add     edi,2 ;dalsi pozice
	dec     cx
	jnz     skzl1 ;opakuj dokola
	skz2:   pop     edi ;obnov edi
	pop     esi ;obnov esi
	mov     edx,[zoom]tzoom.ytable ;vyzvedni ukazatel na ytable
	mov     cx,[edx] ;cx - o kolik pozic se mam v texture posunout dolu
	or      cx,cx
	jz      skzskp
	skzl2:  add     esi,[zoom]tzoom.texture_line ;posun o jednu pozici
	dec     cx ;sniz citac
	jnz     skzl2 ;dokud neni nula
	skzskp:add     edx,2 ;dalsi hodnota v tabulce
	mov     [zoom]tzoom.ytable,edx ;uloaz na puvodni misto
	sub     edi,[zoom]tzoom.line_len ;odecti tolik, kolik odpovida lince na obrazovce
	sub     ecx,10000h ;sniz horni pulku ecx o jedna
	jnz     skzl3 ;opakuj dokud neni nula
	}*/
}

void sikma_zprava(void)
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *scr = (word *)zoom.startptr;
	const word *palette = (word *)zoom.palette;
	word cy = zoom.ycount;
	const unsigned char *pixmap = zoom.texture;
	const short *ytable = zoom.ytable;
	while (cy) {
		const int32_t *xtable = zoom.xtable;
		word cx = zoom.xmax;
		word *scr_iter = scr;
		const unsigned char *pixmap_iter = pixmap;
		while (cx > 0) {
			unsigned char pb = *pixmap_iter++;
			pixmap_iter+=*xtable;
			xtable++;
			if (pb == 1) break;
			if (pb != 0) *scr_iter = palette[pb];
			scr_iter--;
			cx--;
		}
		pixmap+=zoom.texture_line*(*ytable);
		ytable++;
		scr-=scr_linelen2;
		cy--;
	}
/*
	__asm
	{
		mov     edi,zoom       ;nacti ukazatel do obrazovky
			mov     ebx,[zoom]tzoom.palette ;ukazatel na paletu
			mov     cx,short ptr [zoom]tzoom.ycount ;velikost textury na y
			shl     ecx,16  ;vloz do horni pulky ecx
			mov     esi,[zoom]tzoom.texture ;nacti ukazatel na texturu
skzp3:  mov     edx,[zoom]tzoom.xtable ;nacti ukazetel na zvetsovaci tabulku x
		push    esi ;uchovej esi
		push    edi ;uchovej edi
		mov     cx,[zoom]tzoom.xmax
skzp1:  xor     eax,eax ;vynuluj eax pro spravny vypocet
		lodsb       ;nacti bod
		add     esi,[edx] ;posun se od nekolik pozic v texture podle hodnoty v tabulce x
		add     edx,4 ;posun se v tabulce x o dalsi polozku
		or      al,al ;test bodu na nulu
		jz      skz3  ;preskoc transparetni barvu
		cmp     al,1  ;test bodu na jedna
		jz      skz4  ;ukonci kresleni linky pokud narazi na 1
		mov     ax,[eax*2+ebx] ;konverze barvy podle palety
		mov     [edi],ax ;nakresli bod na obrazovce
skz3:   sub     edi,2 ;dalsi pozice
		dec     cx
		jnz     skzp1 ;opakuj dokola
skz4:   pop     edi ;obnov edi
		pop     esi ;obnov esi
		mov     edx,[zoom]tzoom.ytable ;vyzvedni ukazatel na ytable
		mov     cx,[edx] ;cx - o kolik pozic se mam v texture posunout dolu
		or      cx,cx
		jz      skpskp
skzp2:  add     esi,[zoom]tzoom.texture_line ;posun o jednu pozici
		dec     cx ;sniz citac
		jnz     skzp2 ;dokud neni nula
skpskp: add     edx,2 ;dalsi hodnota v tabulce
		mov     [zoom]tzoom.ytable,edx ;uloaz na puvodni misto
		sub     edi,[zoom]tzoom.line_len ;odecti tolik, kolik odpovida lince na obrazovce
		sub     ecx,10000h ;sniz horni pulku ecx o jedna
		jnz     skzp3 ;opakuj dokud neni nula
	}*/
}


void fcdraw(void *source,void *target, void *table)
//#pragma aux fcdraw parm [EDX][EBX][EAX] modify [ECX ESI EDI];
{

	word *src = (word *)source;
	word *trg = (word *)target;
	T_FLOOR_MAP *t = (T_FLOOR_MAP *)table;
	uint32_t cc;

	do {
		word *ss = t->txtrofs/2+src;
		word *tt = t->lineofs/2+trg;
		cc = t->linesize;
		memcpy(tt,ss,cc*2);
		cc = t->counter;
		t++;
	} while (cc != 0);
/*
	__asm
	{
		mov     edx,source
			mov     ebx,target
			mov     eax,table
			;Kresli strop nebo podlahu podle draw_table
			;EDX - sourceTxt
			;EBX - TargerTxt - LineOfset
			;  (Lineofs je pocet bajtu odpovidajici
			;  souradnicim [0,184] pro podlahu nebo [0,0]
		; pro strop)
			;EAX - draw_table
fcdraw_:mov     esi,[eax+12]
		mov     edi,[eax]
		add     edi,ebx
			add     esi,edx
			mov     ecx,[eax+4]
		shr     ecx,1
			rep     movsd
			rcl     ecx,1
			rep     movsw
			mov     ecx,[eax+8]
		add     eax,16
			or      ecx,ecx
			jnz     fcdraw_
	}*/
}

void klicovani_anm_back(void *target,void *source);
void klicovani_anm(void *target,void *source,char mirror)
//#pragma aux klicovani_anm parm [edi][esi][eax] modify [ecx edx ebx]
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *t = (word *)target;
	word *s = (word *)source;
	if (mirror) {
		unsigned int l = 180;
		while (l > 0) {
			unsigned int c = 640;
			while (c > 0) {
				word p = *s++;
				if ((p & 0x8000 ) == 0) {
					p = p + (p & ~0x1F);
					--c;
					t[c] = p;
					t[c+scr_linelen2] = p;
					--c;
					t[c] = p;
					t[c+scr_linelen2] = p;
				}  else {
					c-=2;
				}
			}
			t+=2*scr_linelen2;
			--l;
		}
	} else {

		unsigned int l = 180;
		while (l > 0) {
			unsigned int c = 320;
			while (c > 0) {
				word p = *s++;
				if ((p & 0x8000 ) == 0) {
					p = p + (p & ~0x1F);
					t[0] = p;
					t[scr_linelen2] = p;
					++t;
					t[0] = p;
					t[scr_linelen2] = p;
					++t;
				} else {
					t+=2;
				}
				--c;
			}
			t+=scr_linelen2;
			--l;
		}

	}
}
	/*
		__asm
	{
		mov     edi,target
			mov     esi,source

			mov     cl,180
ka_lp2: mov     ebx,320
ka_lp1: lodsw
		movzx   eax,ax
		test    eax,0x8000
		jnz     ka_skip
		mov     edx,eax
		and     edx,0x7FE0
		add     eax,edx
		mov     edx,eax
		shl     edx,16
		add     edi,scr_linelen
		or      eax,edx
		mov     [edi],eax
		sub     edi,scr_linelen
		mov     [edi],eax
ka_skip:add     edi,4
		dec     ebx
		jnz     ka_lp1
		sub		edi,1280
		add     edi,scr_linelen
		add     edi,scr_linelen
		dec     cl
		jnz     ka_lp2
	}
}

static void klicovani_anm_back(void *target,void *source)
//#pragma aux klicovani_anm parm [edi][esi][eax] modify [ecx edx ebx]
{
	__asm
	{
		mov     edi,target
			mov     esi,source

			mov     ecx,180
			add     edi,1280
kba_lp2: mov     ebx,320
kba_lp1: lodsw
		 sub     edi,4
		 movzx   eax,ax
		 test    eax,0x8000
		 jnz     kba_skip
		 mov     edx,eax
		 and     edx,0x7FE0
		 add     eax,edx
		 mov     edx,eax
		 shl     edx,16
		 add     edi,scr_linelen
		 or      eax,edx
		 mov     [edi],eax
		 sub     edi,scr_linelen
		 mov     [edi],eax
kba_skip:dec     ebx
		 jnz     kba_lp1
		 add     edi,scr_linelen
		 add     edi,scr_linelen
		 add     edi,1280
		 dec     ecx
		 jnz     kba_lp2
	}
}
*/

#define zobraz_1 \
	__asm lodsb \
	__asm movzx   eax,al  \
	__asm movzx   eax,short ptr [eax*2+ebx]  \
	__asm stosw \



void small_anm_buff(void *target,void *buff,void *paleta)
//#pragma aux small_anm_buff parm[edi][esi][ebx] modify [eax ecx]
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *t = (word *)target;
	unsigned char *s = (unsigned char *)buff;
	word *p = (word *)paleta;
	int i,j;

	for ( i = 0; i < 180; i++) {
		for (j = 0; j < 320; j++) {
			*t++ = p[*s++];
		}
		t+=scr_linelen2-320;
	}
/*
	__asm
	{
		mov   edi,target;
		mov   esi,buff;
		mov   ebx,paleta

			mov     ecx,179*10000h
shmab4: mov     cx,320
shmab3: zobraz_1
		dec     cx
		jnz     shmab3
		add     edi,640
		sub     ecx,010000h
		jnc     shmab4
	}*/
}

void small_anm_delta(void *target,void *buff,void *paleta)
//#pragma aux small_anm_delta parm[edi][esi][ebx] modify [eax ecx]
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *t = (word *)target;
	word *pal = (word *)paleta;
	uint32_t *deltastart = (uint32_t *)buff;
	uint32_t ofs = *deltastart++;
	unsigned char *control = (unsigned char *)deltastart;
	unsigned char *pixels = control + ofs;
	int y;

	for (y = 0; y < 180; y++) {
		word *tp = t;
		do {
			unsigned char c = *control++;
			if (c >= 0xc0) {
				//skip line
				unsigned char d = c -  0xc0;
				y+=d;
				t+=(d+1)*scr_linelen2;
				break;
			} else {
				tp+=2*c;
				c = *control++;
				while (c) {
					*tp++ = pal[*pixels++];
					*tp++ = pal[*pixels++];
					c--;
				}
			}
		} while (1);
	}
}

/*

	__asm
	{
		mov   edi,target;
		mov   esi,buff;
		mov   ebx,paleta


			mov     ch,180          ;180 radek
			mov     eax,[esi]       ;vem offset na delta data
			lea     edx,[esi+eax+4]     ;ebp obsahuje tento offset
			lea     esi,[esi+4]     ;esi obsahuje offset na delta control
shmad5: push    edi             ;uchovej zacatek radky
shmad3: lodsb                   ;vem skip hodnotu
		mov     ah,al           ;uchovej ji ah - kvuli destruktivnimu testu
		and     al,not 3fh      ;na posledni 2 bity
		cmp     al,0c0h         ;C0 a vyssi znamenaji ze se preskakuje radek
		jz      shmad1          ;pri pozitivnim testu je preskoc
		movzx   eax,ah          ;vem skip hodnotu do eax
		shl     eax,2           ;na obrazovce *2 bodu *1 zoom *2 za bod (x4)
		add     edi,eax         ;pricti k edi
		lodsb                   ;vem copy hodnotu
		mov     cl,al           ;zaved counter
		xchg    esi,edx         ;prohod ukazatele control a data
shmad2: zobraz_1                ;zobraz bod
		zobraz_1                ;zobraz bod
		dec     cl              ;opakuj tolikrat kolik je copy hodnota
		jnz     shmad2
		xchg    esi,edx         ;prohod ukazatele control a data
		jmp     shmad3          ;a precti dalsi skip hodnotu
shmad1: pop     edi
		and     ah,03fh         ;maskuj spodnich 6 bitu
		inc     ah              ;+1
shmad4: add     edi,scr_linelen
		dec     ch              ;odecti counter radek
		dec     ah              ;odecti counter
		jnz     shmad4          ;a opakuj ah krat
		or      ch,ch           ;je li counter nulovy tak padame
		jnz     shmad5
	}*/



void scroll_and_copy(void *pic,void *slide, void *scr, int _size,int shift, void *lineinfo)
//#pragma aux scroll_and_copy parm[esi][ebx][edi][ecx][edx][eax]
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *srcpc = (word *)pic;
	word *trg = (word *)scr;
	word *sld = (word *)slide;
	int shiftofs = shift*scr_linelen2;
	int lc = _size;
	int *lnfo = (int *)lineinfo;
	int y;

	for (y = 0; y < lc; y++ ) {
		int left = MIN(lnfo[0],lnfo[2]);
		int right = MAX(lnfo[1],lnfo[3]);
		int x;
		for (x = left; x <= right; x++) {

			word c = sld[x] = sld[x+shiftofs];
			if (c & BGSWITCHBIT) {
				trg[x] = srcpc[x];
			} else {
				trg[x] = c;
			}
		}
		sld+=scr_linelen2;
		trg+=scr_linelen2;
		srcpc+=scr_linelen2;
		lnfo+=2;


	}
/*


	__asm
	{
		mov     esi,pic
			mov     ebx,slide
			mov     edi,scr
			mov     ecx,_size
			mov     edx,shift
			mov     eax,lineinfo
			;odscroluje a kopiruje obrazovku do screenu
			;pouzitelne pro titulky
			;0x8000 je transparentni
			;edi screen
			;ebx buffer
			;esi back_picture
			;ecx velikost bufferu (pocet_radku)
			;edx pocet bajtu o kolik je nutne posouvat (*1280)
			;eax - ukazatel na tittle lines
			push    ebp
			mov     ebp,eax         ;uloz ukazatel na titlelines
			mov     eax,edx
			imul    eax,scr_linelen
sac_lp1:push    ecx
		push    eax
		mov     eax,[ebp]       ;nacti zac
		mov     ecx,[ebp+edx*8]
		cmp     eax,ecx
			jc      sac_sk1
			mov     eax,ecx
sac_sk1:shl     eax,1           ;adresa
		add     esi,eax         ;pricti k pointrum
		add     edi,eax
		add     ebx,eax
		add     ebp,4           ;presun se na kon
		shr     eax,1           ;eax je zacatek v bodech
		mov     ecx,[ebp+edx*8] ;vem konec
		cmp     ecx,[ebp]
		jnc     sac_sk2
			mov     ecx,[ebp]
sac_sk2:push    ecx
			sub     ecx,eax         ;kon-zac+1=celk pocet
			xchg    edx,[esp+4]
sac_lp2:mov     eax,[ebx+edx]   ;Vem data na dalsim radku
			mov     [ebx],eax       ;Zapis je sem
			add     ebx,4           ;dalsi pozice
			test    eax,~BGSWITCHBIT ;je to transparentni?
			jz      sac_all         ;pokud ano, zobraz_obrazek
			bt      eax,5          ;test bitu 15
			jnc     sac_1           ;Neni nastaven - zobraz primo barvu
			mov     ax,[esi]        ;jinak vem barvu z obrazku
sac_1:  stosw                   ;zapis barvu
		add     esi,2           ;dalsi pozice
		rol     eax,16          ;presun eax na dolni slovo
		bt      eax,5          ;test bitu 15
		jnc     sac_2           ;neni nastaven - zobraz primo barvu
		mov     ax,[esi]        ;jinak vem barvu z obrazku
sac_2:  stosw                   ;zapis barvu
		add     esi,2           ;dalsi pozice
		jmp     sac_end
sac_all:movsd                   ;presun celeho slova
sac_end:sub     ecx,2           ;odecti counter
		ja      sac_lp2         ;je li nad opakuj
		add     ecx,640         ;pricti sirku, - kolik jsme prejeli
		pop     eax
		sub     ecx,eax       ;odecti konec -> kolik preskocit
		add     ebp,4           ;aktualizuj ukazatele
		shl     ecx,1           ;v adresach
		add     esi,ecx
		push    edx
		mov     edx,scr_linelen
		sub     edx,1280
		add     ecx,edx
		add     edi,ecx
		add     ebx,ecx
		pop     edx
		pop     eax
		xchg    edx,eax
		pop     ecx             ;obnov ecx
		dec     ecx             ;sniz citac radku
		jnz     sac_lp1         ;dokud neni nula
		pop     ebp             ;obnov ebp

	}
	*/
}

#define pic_start 2+2+2+512*5+512*5
#define ed_stack 800*4+600*4
#define ed_stk1 600*4


void enemy_draw(void *src,void *trg,int shade,int scale,int maxspace,int clip)
//#pragma aux enemy_draw parm[ESI][EDI][EBX][EDX][EAX][ECX]
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *picinfo = (word *)src;
	word *screen = (word *)trg;
	int xtable[800];
	int ytable[1200];
	int xcount;

	word pcx = picinfo[0];
	word pcy = picinfo[1];
	unsigned char *picdata = (unsigned char *)src + pic_start;
	word *palette = picinfo+shade/2;
	int clipl = clip & 0xFFFF;
	int clipr = clip >> 16;
	int yiter;

	if (maxspace >= 470) return;

	//prepare ytable;
	{
		int rwofs = pcx*(pcy-1);
		int accu = 0;
		int *w = ytable;
		int sp = maxspace*2;

		while (sp > 0 && rwofs >= 0) {
			*w++ = rwofs;
			accu+=320;
			while (accu >= scale && rwofs >= 0) {
				accu-=scale;
				rwofs-=pcx;
			}
			sp--;
		}
		*w++=-1;
		//ytable je hotova - obsahuje ofsety v obrazku pro kazdou radku a na konci je -1
	}

	//prepare xtable
	{
		int *w = xtable;
		int ofs = 0;
		int accu = 0;
		while (ofs < pcx) {
			*w++ = ofs;
			accu+=320;
			while (accu >= scale) {
				accu-=scale;
				ofs++;
			}
		}
		xcount = w - xtable;
		*w++=-1;
		//xtable je hotova - obsahuje offset v obrazku pro kazdy sloupec a na konci je -1
	}

	if (clipl > xcount) return;
	yiter = 0;
	while (ytable[yiter] >= 0) {
		int xiter = clipl;
		int ofs = ytable[yiter];
		unsigned char *row = picdata+ofs;
		while (xiter < clipr && xtable[xiter] >= 0 ) {
			int xpos = xiter;
			unsigned char p = row[xtable[xiter]];
			if (p != 0) {
				if (p == 1) screen[xpos] = (screen[xpos] & 0xF7DE) >> 1;
				else screen[xpos] = palette[p];
			}
			++xiter;
		}

		screen-=scr_linelen2;
		++yiter;
	}

/*

	__asm
	{
		mov esi,src
			mov edi,trg
			mov ebx,shade
			mov edx,scale
			mov eax,maxspace
			mov ecx,clip

			cmp     eax,470
			jc      ed_ok1
			ret
ed_ok1: push    ebp
		sub     esp,ed_stack ;vyhrad zasobnik
		mov     ebp,esp
		push    ecx          ;uchovej ecx               +1
		push    edi          ;uchovej edi               +2
		push    ebx          ;uchovej ebx               +3
		mov     ebx,eax      ;volne misto je ted v ebx
		mov     edi,ebp      ;nastav edi na zacatek tabulek
		push    ebp          ;                          +4
		movzx   ecx,short ptr[esi]     ;precti xs
		movzx   eax,short ptr[esi+2]   ;precti ys
		imul    eax,ecx      ;xs*ys
		sub     eax,ecx      ;minus 1 radek
		xor     ebp,ebp
ed_lp1b:stosd                ;zapis hodnotu do tabulky
		add     ebp,320      ;zmensovani
ed_lp1a:cmp     ebp,edx
		jc      ed_nxt1      ;pri ebp<edx pokracuj dal
		sub     ebp,edx
		sub     eax,ecx      ;odecti radku
		jbe     ed_end1      ;pokud je to posledni tak konec1
		jmp     ed_lp1a
ed_nxt1:dec     ebx
		jnz     ed_lp1b
ed_end1:xor     eax,eax
		dec     eax
		stosd                ;tabulka pro y je hotova
		pop     ebp          ;                          +3
		lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
		xor     eax,eax      ;vynuluj vypoctove registry
		xor     ebx,ebx
		movzx   ecx,short ptr[esi]  ;ecx je velikost obrazku v x
ed_nxt2:stosd                ;zapis x souradnici
		add     ebx,320      ;pocitej zmenseni
ed_lp2a:cmp     ebx,edx
		jc      ed_nxt2
		sub     ebx,edx
		inc     eax
		cmp     eax,ecx      ;pokud je x>=velikosti obrazku tak konec
		jc     ed_lp2a
		xor     eax,eax
		dec     eax          ;zapis -1 na konec tabulky
		pop     ebx          ;                          +2
		stosd
		mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
		pop     edi          ;obnov registry edi exc ebx+1
		add     ebx,esi      ;najdi adresu prekladove tabulky barev
		add     ebx,2        ;preskoc transparentni barvu
		add     esi,pic_start;presun se na zacatek obrazku
		pop     ecx          ;                          +0
		movzx   edx,cx       ;vem levy okraj
		lea     edx,[ebp+edx*4]
		lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
			cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
			jbe     ed_err
			shr     ecx,16       ;ecx ted obsahuje pravy okraj
ed_lp4: push    esi          ;uchovej esi               +1
		push    edi          ;uchovej edi               +2
		add     esi,[ebp]    ;spocitej spravnou hodnotu esi
		push    ecx          ;uchovej pravy okraj       +3
		push    edx          ;uchovej ukazatel na tabulku+4
ed_lp3: mov     eax,[edx]
		cmp     eax,-1       ;testuj konec tabulky
			jz      ed_end3      ;pri pozitivnim vysledku => konec radky
			movzx   eax,byte ptr[esi+eax];vem barvu
			or      al,al
			jz      ed_skp1      ;preskoc transparentni barvu
			dec     al
			jz      ed_shd       ;1=shadow
			mov     eax,[ebx+eax*2];vyzvedni hicolor
			stosw                ;zapis barvu
			jmp     ed_skp2
ed_shd: movzx   eax,short ptr[edi];vem barvu
		and     eax,0xF7DE   ;stmav
		shr     eax,1
		stosw               ;zapis
		jmp     ed_skp2     ;skok na konec
ed_skp1:add     edi,2       ;preskoc bod
ed_skp2:add     edx,4       ;dalsi pozice
		dec     ecx         ;dokud nedosahneme praveho okraje obrazku
		jnz     ed_lp3
ed_end3:pop     edx         ;obnov vse ulozene
		pop     ecx
		pop     edi
		pop     esi         ;                           +0
		add     ebp,4       ;dalsi y souradnice
		sub     edi,scr_linelen ;dalsi radka
		cmp     dword ptr [ebp],-1 ;test na konec tabulky
		jnz     ed_lp4      ;jinak pokracuj
ed_err: add     esp,ed_stack;vymaz tabulku
		pop     ebp         ;obnov ebp
	}
	*/
}
void enemy_draw_transp(void *src,void *trg,void *shade,int scale,int maxspace,int clip)
//#pragma aux enemy_draw_transp parm[ESI][EDI][EBX][EDX][EAX][ECX]
{

    int32_t scr_linelen2 = GetScreenPitch();
	word *picinfo = (word *)src;
	word *screen = (word *)trg;
	int xtable[800];
	int ytable[1200];
	int xcount;
	word pcx = picinfo[0];
	word pcy = picinfo[1];
	word type = picinfo[2];
	unsigned char *picdata = (unsigned char *)src+(type == (512+8)?6:pic_start);
	word *palette = (word *)shade;
	int clipl = clip & 0xFFFF;
	int clipr = clip >> 16;
	int yiter;

	if (maxspace >= 470) return;

	//prepare ytable;
	{
		int rwofs = pcx*(pcy-1);
		int accu = 0;
		int *w = ytable;
		int sp = maxspace*2;

		while (sp > 0 && rwofs >= 0) {
			*w++ = rwofs;
			accu+=320;
			while (accu >= scale && rwofs >= 0) {
				accu-=scale;
				rwofs-=pcx;
			}
			sp--;
		}
		*w++=-1;
		//ytable je hotova - obsahuje ofsety v obrazku pro kazdou radku a na konci je -1
	}

	//prepare xtable
	{
		int *w = xtable;
		int ofs = 0;
		int accu = 0;
		while (ofs < pcx) {
			*w++ = ofs;
			accu+=320;
			while (accu >= scale) {
				accu-=scale;
				ofs++;
			}
		}
		xcount = w - xtable;
		*w++=-1;
		//xtable je hotova - obsahuje offset v obrazku pro kazdy sloupec a na konci je -1
	}

	if (clipl > xcount) return;
	yiter = 0;
	while (ytable[yiter] >= 0) {
		int xiter = clipl;
		int ofs = ytable[yiter];
		unsigned char *row = picdata+ofs;
		while (xiter < clipr && xtable[xiter] >= 0 ) {
			int xpos = xiter -clipl;
			unsigned char p = row[xtable[xiter]];
			if (p != 0) {
				if (p & 0x80) screen[xpos] = ((screen[xpos] & 0xF7DE) + (palette[p] & 0xF7DE))>>1;
				else screen[xpos] = palette[p];
			}
			++xiter;
		}

		screen-=scr_linelen2;
		++yiter;
	}

/*	__asm
	{
		mov esi,src
			mov edi,trg
			mov ebx,shade
			mov edx,scale
			mov eax,maxspace
			mov ecx,clip

			cmp     eax,470
			jc      et_ok1
			ret
et_ok1: push    ebp
		sub     esp,ed_stack ;vyhrad zasobnik
		mov     ebp,esp
		push    ecx          ;uchovej ecx               +1
		push    edi          ;uchovej edi               +2
		push    ebx          ;uchovej ebx               +3
		mov     ebx,eax      ;volne misto je ted v ebx
		mov     edi,ebp      ;nastav edi na zacatek tabulek
		push    ebp          ;                          +4
		movzx   ecx,short ptr[esi]     ;precti xs
		movzx   eax,short ptr[esi+2]   ;precti ys
		imul    eax,ecx      ;xs*ys
		sub     eax,ecx      ;minus 1 radek
		xor     ebp,ebp
et_lp1b:stosd                ;zapis hodnotu do tabulky
		add     ebp,320      ;zmensovani
et_lp1a:cmp     ebp,edx
		jc      et_nxt1      ;pri ebp<edx pokracuj dal
		sub     ebp,edx
		sub     eax,ecx      ;odecti radku
		jbe     et_end1      ;pokud je to posledni tak konec1
		jmp     et_lp1a
et_nxt1:dec     ebx
		jnz     et_lp1b
et_end1:xor     eax,eax
		dec     eax
		stosd                ;tabulka pro y je hotova
		pop     ebp          ;                          +3
		lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
		xor     eax,eax      ;vynuluj vypoctove registry
		xor     ebx,ebx
		movzx   ecx,short ptr[esi]  ;ecx je velikost obrazku v x
et_nxt2:stosd                ;zapis x souradnici
		add     ebx,320      ;pocitej zmenseni
et_lp2a:cmp     ebx,edx
		jc      et_nxt2
		sub     ebx,edx
		inc     eax
		cmp     eax,ecx      ;pokud je x>=velikosti obrazku tak konec
		jc     et_lp2a
		xor     eax,eax
		dec     eax          ;zapis -1 na konec tabulky
		pop     ebx          ;                          +2
		stosd
		mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
		pop     edi          ;obnov registry edi exc ebx+1
		cmp     byte ptr [esi+5],2      ;obrazek bez palety?
		jz      et_pl1
		add     esi,pic_start;presun se na zacatek obrazku (za paletu)
		jmp     et_pl2
et_pl1: add     esi,6        ;(obrazek bez palety, presun se za hlavicku)
et_pl2: pop     ecx          ;                          +0
		movzx   edx,cx       ;vem levy okraj
		lea     edx,[ebp+edx*4]
		lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
			cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
			jbe     et_err
			shr     ecx,16       ;ecx ted obsahuje pravy okraj
et_lp4: push    esi          ;uchovej esi               +1
		push    edi          ;uchovej edi               +2
		add     esi,[ebp]    ;spocitej spravnou hodnotu esi
		push    ecx          ;uchovej pravy okraj       +3
		push    edx          ;uchovej ukazatel na tabulku+4
et_lp3: mov     eax,[edx]
		cmp     eax,-1       ;testuj konec tabulky
			jz      et_end3      ;pri pozitivnim vysledku => konec radky
			movzx   eax,byte ptr[esi+eax];vem barvu
			test    al,80h
			jnz     et_shd
			or      al,al
			jz      et_skp1      ;preskoc transparentni barvu
			mov     eax,[ebx+eax*2];vyzvedni hicolor
			stosw                ;zapis barvu
			jmp     et_skp2
et_shd: and     short ptr[edi],0xF7DE ;1111 0111 1101 1110
		mov     eax,[ebx+eax*2];vyzvedni hicolor
		and     eax,0xF7DE  ;stmav
		add     ax,short ptr[edi]
		rcr     ax,1
			stosw               ;zapis
			jmp     et_skp2     ;skok na konec
et_skp1:add     edi,2       ;preskoc bod
et_skp2:add     edx,4       ;dalsi pozice
		dec     ecx         ;dokud nedosahneme praveho okraje obrazku
		jnz     et_lp3
et_end3:pop     edx         ;obnov vse ulozene
		pop     ecx
		pop     edi
		pop     esi         ;                           +0
		add     ebp,4       ;dalsi y souradnice
		sub     edi,scr_linelen    ;dalsi radka
		cmp     dword ptr [ebp],-1 ;test na konec tabulky
		jnz     et_lp4      ;jinak pokracuj
et_err: add     esp,ed_stack;vymaz tabulku
		pop     ebp         ;obnov ebp
	}*/
}

void enemy_draw_mirror_transp(void *src,void *trg,void *shade,int scale,int maxspace,int clip)
//#pragma aux enemy_draw_mirror_transp parm[ESI][EDI][EBX][EDX][EAX][ECX]
{

    int32_t scr_linelen2 = GetScreenPitch();
	word *picinfo = (word *)src;
	word *screen = (word *)trg;
	int xtable[800];
	int ytable[1200];
	int xcount;
	word pcx = picinfo[0];
	word pcy = picinfo[1];
	word type = picinfo[2];
	unsigned char *picdata = (unsigned char *)src+(type == (512+8)?6:pic_start);
	word *palette = (word *)shade;
	int clipl = clip & 0xFFFF;
	int clipr = clip >> 16;
	int yiter;

	if (maxspace >= 470) return;

	//prepare ytable;
	{
		int rwofs = pcx*(pcy-1);
		int accu = 0;
		int *w = ytable;
		int sp = maxspace*2;

		while (sp > 0 && rwofs >= 0) {
			*w++ = rwofs;
			accu+=320;
			while (accu >= scale && rwofs >= 0) {
				accu-=scale;
				rwofs-=pcx;
			}
			sp--;
		}
		*w++=-1;
		//ytable je hotova - obsahuje ofsety v obrazku pro kazdou radku a na konci je -1
	}

	//prepare xtable
	{
		int *w = xtable;
		int ofs = pcx-1;
		int accu = 0;
		while (ofs >= 0) {
			*w++ = ofs;
			accu+=320;
			while (accu >= scale) {
				accu-=scale;
				ofs--;
			}
		}
		xcount = w - xtable;
		*w++=-1;
		//xtable je hotova - obsahuje offset v obrazku pro kazdy sloupec a na konci je -1
	}

	if (clipl > xcount) return;
	yiter = 0;
	while (ytable[yiter] >= 0) {
		int xiter = clipl;
		int ofs = ytable[yiter];
		unsigned char *row = picdata+ofs;
		while (xiter < clipr && xtable[xiter] >= 0 ) {
			int xpos = xiter -clipl;
			unsigned char p = row[xtable[xiter]];
			if (p != 0) {
				if (p & 0x80) screen[xpos] = ((screen[xpos] & 0xF7DE) + (palette[p] & 0xF7DE))>>1;
				else screen[xpos] = palette[p];
			}
			++xiter;
		}

		screen-=scr_linelen2;
		++yiter;
	}

	/*
	__asm
	{
		mov esi,src
			mov edi,trg
			mov ebx,shade
			mov edx,scale
			mov eax,maxspace
			mov ecx,clip

			push    ebp
			sub     esp,ed_stack ;vyhrad zasobnik
			mov     ebp,esp
			push    ecx          ;uchovej ecx               +1
			push    edi          ;uchovej edi               +2
			push    ebx          ;uchovej ebx               +3
			mov     ebx,eax      ;volne misto je ted v ebx
			mov     edi,ebp      ;nastav edi na zacatek tabulek
			push    ebp          ;                          +4
			movzx   ecx,short ptr[esi]     ;precti xs
			movzx   eax,short ptr[esi+2]   ;precti ys
			imul    eax,ecx      ;xs*ys
			sub     eax,ecx      ;minus 1 radek
			xor     ebp,ebp
etmlp1b:stosd                ;zapis hodnotu do tabulky
		add     ebp,320      ;zmensovani
etmlp1a:cmp     ebp,edx
		jc      etmnxt1      ;pri ebp<edx pokracuj dal
		sub     ebp,edx
		sub     eax,ecx      ;odecti radku
		jbe     etmend1      ;pokud je to posledni tak konec1
		jmp     etmlp1a
etmnxt1:dec     ebx
		jnz     etmlp1b
etmend1:xor     eax,eax
		dec     eax
		stosd                ;tabulka pro y je hotova
		pop     ebp          ;                          +3
		lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
		xor     ebx,ebx
		movzx   ecx,short ptr[esi]  ;ecx je velikost obrazku v x
		mov     eax,ecx      ;eax je velikost obrazku v x
		dec     eax
etmnxt2:stosd                ;zapis x souradnici
		add     ebx,320      ;pocitej zmenseni
etmlp2a:cmp     ebx,edx
		jc      etmnxt2
		sub     ebx,edx
		sub     eax,1
		jnc     etmlp2a
		xor     eax,eax
		dec     eax          ;zapis -1 na konec tabulky
		pop     ebx          ;                          +2
		stosd
		mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
		pop     edi          ;obnov registry edi exc ebx+1
		cmp     byte ptr [esi+5],2      ;obrazek bez palety?
		jz      etmpl1
		add     esi,pic_start;presun se na zacatek obrazku (za paletu)
		jmp     etmpl2
etmpl1: add     esi,6        ;(obrazek bez palety, presun se za hlavicku)
etmpl2: pop     ecx          ;                          +0
		movzx   edx,cx       ;vem levy okraj
		lea     edx,[ebp+edx*4]
		lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
			cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
			jbe     etmerr
			shr     ecx,16       ;ecx ted obsahuje pravy okraj
etmlp4: push    esi          ;uchovej esi               +1
		push    edi          ;uchovej edi               +2
		add     esi,[ebp]    ;spocitej spravnou hodnotu esi
		push    ecx          ;uchovej pravy okraj       +3
		push    edx          ;uchovej ukazatel na tabulku+4
etmlp3: mov     eax,[edx]
		cmp     eax,-1       ;testuj konec tabulky
			jz      etmend3      ;pri pozitivnim vysledku => konec radky
			movzx   eax,byte ptr[esi+eax];vem barvu
			test    al,80h
			jnz     etmshd
			or      al,al
			jz      etmskp1      ;preskoc transparentni barvu
			mov     eax,[ebx+eax*2];vyzvedni hicolor
			stosw                ;zapis barvu
			jmp     etmskp2
etmshd: and     short ptr[edi],0xF7DE
		mov     eax,[ebx+eax*2];vyzvedni hicolor
		and     eax,0xF7DE   ;stmav
		add     ax,short ptr[edi]
		rcr     ax,1
			stosw               ;zapis
			jmp     etmskp2     ;skok na konec
etmskp1:add     edi,2       ;preskoc bod
etmskp2:add     edx,4       ;dalsi pozice
		dec     ecx         ;dokud nedosahneme praveho okraje obrazku
		jnz     etmlp3
etmend3:pop     edx         ;obnov vse ulozene
		pop     ecx
		pop     edi
		pop     esi         ;                           +0
		add     ebp,4       ;dalsi y souradnice
		sub     edi,scr_linelen    ;dalsi radka
		cmp     dword ptr [ebp],-1 ;test na konec tabulky
		jnz     etmlp4      ;jinak pokracuj
etmerr: add     esp,ed_stack;vymaz tabulku
		pop     ebp         ;obnov ebp
	}
	*/
}
void enemy_draw_mirror(void *src,void *trg,int shade,int scale,int maxspace,int clip)
//#pragma aux enemy_draw_mirror parm[ESI][EDI][EBX][EDX][EAX][ECX]
//clip je v poradi vpravo - vlevo (HiLo)
{
    int32_t scr_linelen2 = GetScreenPitch();
	word *picinfo = (word *)src;
	word *screen = (word *)trg;
	int xtable[800];
	int ytable[1200];
	int xcount;
	word pcx = picinfo[0];
	word pcy = picinfo[1];
	unsigned char *picdata = (unsigned char *)src + pic_start;
	word *palette = picinfo+shade/2;
	int clipl = clip & 0xFFFF;
	int clipr = clip >> 16;
	int yiter;

	if (maxspace >= 470) return;

	//prepare ytable;
	{
		int rwofs = pcx*(pcy-1);
		int accu = 0;
		int *w = ytable;
		int sp = maxspace*2;

		while (sp > 0 && rwofs >= 0) {
			*w++ = rwofs;
			accu+=320;
			while (accu >= scale && rwofs >= 0) {
				accu-=scale;
				rwofs-=pcx;
			}
			sp--;
		}
		*w++=-1;
		//ytable je hotova - obsahuje ofsety v obrazku pro kazdou radku a na konci je -1
	}

	//prepare xtable
	{
		int *w = xtable;
		int ofs = pcx-1;
		int accu = 0;
		while (ofs >= 0) {
			*w++ = ofs;
			accu+=320;
			while (accu >= scale) {
				accu-=scale;
				ofs--;
			}
		}
		xcount = w - xtable;
		*w++=-1;
		//xtable je hotova - obsahuje offset v obrazku pro kazdy sloupec a na konci je -1
	}

	if (clipl > xcount) return;
	yiter = 0;
	while (ytable[yiter] >= 0) {
		int xiter = clipl;
		int ofs = ytable[yiter];
		unsigned char *row = picdata+ofs;
		while (xiter < clipr && xtable[xiter] >= 0 ) {
			int xpos = xiter;
			unsigned char p = row[xtable[xiter]];
			if (p != 0) {
				if (p == 1) screen[xpos] = (screen[xpos] & 0xF7DE) >> 1;
				else screen[xpos] = palette[p];
			}
			++xiter;
		}

		screen-=scr_linelen2;
		++yiter;
	}

	/*

	__asm
	{
		mov esi,src
			mov edi,trg
			mov ebx,shade
			mov edx,scale
			mov eax,maxspace
			mov ecx,clip

			push    ebp
			sub     esp,ed_stack ;vyhrad zasobnik
			mov     ebp,esp
			push    ecx          ;uchovej ecx               +1
			push    edi          ;uchovej edi               +2
			push    ebx          ;uchovej ebx               +3
			mov     ebx,eax      ;volne misto je ted v ebx
			mov     edi,ebp      ;nastav edi na zacatek tabulek
			push    ebp          ;                          +4
			movzx   ecx,short ptr[esi]     ;precti xs
			movzx   eax,short ptr[esi+2]   ;precti ys
			imul    eax,ecx      ;xs*ys
			sub     eax,ecx      ;minus 1 radek
			xor     ebp,ebp
edmlp1b:stosd                ;zapis hodnotu do tabulky
		add     ebp,320      ;zmensovani
edmlp1a:cmp     ebp,edx
		jc      edmnxt1      ;pri ebp<edx pokracuj dal
		sub     ebp,edx
		sub     eax,ecx      ;odecti radku
		jbe     edmend1      ;pokud je to posledni tak konec1
		jmp     edmlp1a
edmnxt1:dec     ebx
		jnz     edmlp1b
edmend1:xor     eax,eax
		dec     eax
		stosd                ;tabulka pro y je hotova
		pop     ebp          ;                          +3
		lea     edi,[ebp+ed_stk1] ;nastav se na druhou tabulku
		xor     ebx,ebx
		movzx   ecx,short ptr[esi]  ;ecx je velikost obrazku v x
		mov     eax,ecx      ;eax je velikost obrazku v x
		dec     eax
edmnxt2:stosd                ;zapis x souradnici
		add     ebx,320      ;pocitej zmenseni
edmlp2a:cmp     ebx,edx
		jc      edmnxt2
		sub     ebx,edx
		sub     eax,1
		jnc     edmlp2a
		xor     eax,eax
		dec     eax          ;zapis -1 na konec tabulky
		pop     ebx          ;                          +2
		stosd
		mov     eax,edi      ;konecnou pozici pro tabulku x zapis do eax
		pop     edi          ;obnov registry edi exc ebx+1
		add     ebx,esi      ;najdi adresu prekladove tabulky barev
		add     ebx,2        ;preskoc transparentni barvu
		add     esi,pic_start;presun se na zacatek obrazku
		pop     ecx          ;                          +0
		movzx   edx,cx       ;vem levy okraj
		lea     edx,[ebp+edx*4]
		lea     edx,[edx+ed_stk1];edx ukazuje na sloupce
			cmp     eax,edx      ;je-li levy okraj za platnou tabulkou tak konec
			jbe     edmerr
			shr     ecx,16       ;ecx ted obsahuje pravy okraj
edmlp4: push    esi          ;uchovej esi               +1
		push    edi          ;uchovej edi               +2
		add     esi,[ebp]    ;spocitej spravnou hodnotu esi
		push    ecx          ;uchovej pravy okraj       +3
		push    edx          ;uchovej ukazatel na tabulku+4
edmlp3: mov     eax,[edx]
		cmp     eax,-1       ;testuj konec tabulky
			jz      edmend3      ;pri pozitivnim vysledku => konec radky
			movzx   eax,byte ptr[esi+eax];vem barvu
			or      al,al
			jz      edmskp1      ;preskoc transparentni barvu
			dec     al
			jz      edmshd       ;1=shadow
			mov     eax,[ebx+eax*2];vyzvedni hicolor
			stosw                ;zapis barvu
			jmp     edmskp2
edmshd: movzx   eax,short ptr[edi];vem barvu
		and     eax,0xF7DE   ;stmav
		shr     eax,1
		stosw               ;zapis
		jmp     edmskp2     ;skok na konec
edmskp1:add     edi,2       ;preskoc bod
edmskp2:add     edx,4       ;dalsi pozice
		dec     ecx         ;dokud nedosahneme praveho okraje obrazku
		jnz     edmlp3
edmend3:pop     edx         ;obnov vse ulozene
		pop     ecx
		pop     edi
		pop     esi         ;                           +0
		add     ebp,4       ;dalsi y souradnice
		sub     edi,scr_linelen  ;dalsi radka
		cmp     dword ptr [ebp],-1 ;test na konec tabulky
		jnz     edmlp4      ;jinak pokracuj
edmerr: add     esp,ed_stack;vymaz tabulku
		pop     ebp         ;obnov ebp
	}
	*/
}
