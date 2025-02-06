#ifndef ___CZTABLES__H___
#define ___CZTABLES__H___

#ifdef __cplusplus
extern "C" {
#endif


void windows2kamenik(const char *src, int size, char *trg);
void kamenik2windows(const char *src, int size, char *trg);
int windows2kamenik_chr(int chr);
int kamenik2windows_chr(int chr);



#ifdef __cplusplus
  }
#endif

#endif
