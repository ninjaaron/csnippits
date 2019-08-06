#ifndef BUFFERED_READER
#define BUFFERED_READER

#if defined(__cplusplus)
extern "C" {
#endif
 
	#include <stddef.h>
	typedef struct BufferedReader_s BufferedReader;

	BufferedReader *br_new(FILE *stream, size_t buffsize);
	BufferedReader *br_open(char *name, size_t buffsize);
	void br_close(BufferedReader *br);
	int br_eof(BufferedReader *br);
	int br_error(BufferedReader *br);
	int br_getc(BufferedReader *br, size_t keep);
	void br_rewind(BufferedReader *br);
	char *br_strptr(BufferedReader *br, size_t length);
 
#if defined(__cplusplus)
} /* extern "C" */
#endif
 
#endif
