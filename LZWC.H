/****************************************************************
 *         LL        ZZZZZ      WW    WW                        *
 *         LL           ZZ      WW    WW   Autor: Ondrej Novak  *
 *         LL          ZZ       WW    WW   Rok vzniku: 1998     *
 *         LL         ZZ        WW WW WW   Prava: Public Domain *
 *         LL        ZZ          WW  WW                         *
 *         LLLLLL    ZZZZZ       WW  WW                         *
 ****************************************************************

Knihovna LZWC je komprimacni a dekomprimacni knihovna metodou LZW upravenou
rozdilovym kodovanim. Jeji ovladani je jednoduche.
*/
char init_lzw_compressor(int dic_size);
/*
Inicializuje knihovnu. Inicializovat je nutne pred kazdym dalsim pouzitim.
"dic_size" je pocatecni velikost slovniku a zaroven tak udava bitovou
hloubku komprimovanych (dekomprimovanych dat). Tato verze MUSI tuto hodnotu
nastavovat na 8. Jina hodnota neni pripustna. Vyuziti se predpoklada
v budoucnu, kdy bude mozne komprimovat i jinou bitovou hloubku.
Vraci -1 pokud neco selhalo.
*/
void done_lzw_compressor();
/*
Ukoncuje praci s knihovnou, maze slovniky, uvolnuje pamet.
*/

long lzw_encode(unsigned char *source,void *target,int size);
/*
Komprimuje data, jejichz ukazatel je ulozen v "source"
(pretypujte na unsigned char). Tyto data maji velikost "size"
Dale je nutne rezervovat pamet "target" ktera by mela mit velikost aspon
o 16 bytu vetsi nez size. Pamet "target" je nutne predtim VYNULOVAT!!!
Funkce vraci velikost zakomprimovanych dat, nebo -1 pokud se nepodarilo
data z komprimovat. I v tomto pripade je nutne vzit na vedomi
ze komprese probehla, byt neuspesne.
*/

long lzw_decode(void *source,unsigned char *target);
/*
Dekomprimuje data, jejichz ukazatel je ulozen v "source".
Dale rezervujte dostatecne mnozstvi pameti v "target"
Funkce vraci velikost dekomprimovanych dat, nebo -1 pokud volani selhalo.
*/

void reinit_lzw();
/*
Funkce maze slovniky, ale bez nutnosti knihovnu znovu inicializovat
*/

/*
DULEZITA POZNAMKA!!!
Po kazdem volani decode a encode je nutne knihovnu reinicializovat
 (popripade ukoncit)

Nejvetsi komprimovatelna velikost je 0x10000000 bytes
*/
