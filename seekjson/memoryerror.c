#include <stdio.h>
#include <stdlib.h>

void memoryerror(void)
{
	fputs("could not allocate memory\n", stderr);
	exit(1);
}
