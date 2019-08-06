#ifndef BUFFERED_READER
#define BUFFERED_READER

#ifndef _STDIO_H
#define _STDIO_H 1
 
#if defined(__cplusplus)
extern "C" {
#endif
 
	typedef struct BufferedReader_s BufferedReader;

	BufferedReader *br_new(FILE *stream, size_t buffsize);
	BufferedReader *br_open(char *name, size_t buffsize);
	void br_close(BufferedReader *br);
	int br_eof(BufferedReader *br);
	int br_error(BufferedReader *br);
	int br_getc(BufferedReader *br, size_t keep);
	char *br_getbuff(BufferedReader *br);
 
#if defined(__cplusplus)
} /* extern "C" */
#endif
 
#endif
#endif
