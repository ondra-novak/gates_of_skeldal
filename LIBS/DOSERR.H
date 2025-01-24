/*void install_dos_error(void *,void *);
#pragma aux install_dos_error parm [eax][edx] modify [ebx ecx esi edi]
*/

#define _ERR_WRITE 1
#define _ERR_SYS 0
#define _ERR_FAT 1
#define _ERR_DIR 2
#define _ERR_DATA 3
#define _ERR_EFAIL 8
#define _ERR_ERETRY 16
#define _ERR_EIGNORE 32
#define _ERR_NOTDISK 128

#define _ERR_IGNORE 0
#define _ERR_RETRY 1
#define _ERR_ABORT 2
#define _ERR_FAIL 3
