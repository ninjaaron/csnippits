#ifdef DEBUG

#define debug(string) \
	fprintf(stderr, " \x1b[31mDEBUG\x1b[0m: %s\n", string)
#define debugf(fmtstring, ...) \
	fprintf(stderr, " \x1b[31mDEBUG\x1b[0m:" fmtstring "\n", __VA_ARGS__)

#else

#define debug(string)
#define debugf(fmtstring, ...)

#endif


#ifndef MEMORY_ERROR_H
#define MEMORY_ERROR_H
	
#include <stdio.h>

void memoryerror(void);
 
#endif
