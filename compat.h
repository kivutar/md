#include <stdio.h>

#define sysfatal printf
#define print printf
#define seek lseek
#define nil NULL
#define OWRITE O_WRONLY
#define ORDWR O_RDWR
#define OREAD O_RDONLY
#define OEXCL O_EXCL
#define	nelem(x) (sizeof(x)/sizeof((x)[0]))

u64int keys, keys2;
int trace, paused;
int savereq, loadreq;
int scale, fixscale, warp10;
uchar *pic;