void init_lzw_compressor(int dic_size);
void done_lzw_compressor();
long lzw_encode(char *source,void *target,int size);
void lzw_decode(void *source,char *target);
void reinit_lzw();
