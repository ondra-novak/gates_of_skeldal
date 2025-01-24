#define concat(c,s1,s2) \
        c=alloca(strlen(s1)+strlen(s2)+1);\
        strcpy(c,s1);\
        strcat(c,s2)

extern word boldcz;
extern word ikones;
extern void *icones;
extern void *vga_font;
extern word sipka;

extern int win_preview;

void shutdown();

#define crash(msg) \
           {\
           shutdown();\
           printf("Module: %s\nLine: %d\nMessage: %s\n",__FILE__,__LINE__,msg);\
           abort();\
           }\

#define WINCOLOR 0x631f
#define LABELCOLOR 0x0f
#define BUTTONCOLOR (28*1024+24*32+3)
#define PRG_HEADER "MGIF Sound editor v1."VERSION


void show_full_lfb12e(void *source,void *target,void *palette);
#pragma aux show_full_lfb12e parm[esi][edi][ebx] modify [eax ecx edx]
void show_delta_lfb12e(void *source,void *target,void *palette);
#pragma aux show_delta_lfb12e parm[esi][edi][ebx] modify [eax ecx edx]
void show_full_interl_lfb130(void *source,void *target,void *palette);
#pragma aux show_full_interl_lfb130 parm[esi][edi][ebx] modify [eax ecx edx]
void show_delta_interl_lfb130(void *source,void *target,void *palette);
#pragma aux show_delta_interl_lfb130 parm[esi][edi][ebx] modify [eax ecx edx]
char test_next_frame(void *bufpos,int size);
#pragma aux test_next_frame parm [edi][ecx] modify [ebx] value [al]
void *sound_decompress(void *source,void *bufpos,int size,void *ampl_tab);
#pragma aux sound_decompress parm [esi][edi][ecx][ebx] modify [eax edx] value [edi]
