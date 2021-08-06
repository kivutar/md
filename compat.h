#include <stdio.h>
#include <stdlib.h>

#define sysfatal(fmt, ...){printf(fmt"\n", ##__VA_ARGS__); exit(EXIT_FAILURE);}
#define print printf
#define seek lseek
#define nil NULL
#define OWRITE O_WRONLY
#define ORDWR O_RDWR
#define OREAD O_RDONLY
#define OEXCL O_EXCL
#define	nelem(x) (sizeof(x)/sizeof((x)[0]))

int readn(int f, void *data, int len);
