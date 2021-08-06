#include <unistd.h>
#include "u.h"
#include "compat.h"

int
readn(int f, void *data, int len)
{
	uchar *p, *e;

	p = data;
	e = p + len;
	while(p < e){
		if((len = read(f, p, e - p)) <= 0)
			break;
		p += len;
	}
	return p - (uchar*)data;
}