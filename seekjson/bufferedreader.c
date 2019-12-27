/*
 *  A FILE buffer that allows holding on to some of the previous
 *  content when it refils and rewinding to content you've already
 *  seen. Especially useful if the file is a stream.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

/* #define DEBUG */
#include "memoryerror.h"

typedef struct BufferedReader_s {
	FILE *stream;
	char *buff;
	size_t buffsize;
	size_t at;       /* current position in the buffer */
	bool end;        /* set to true when the file is ended */
} BufferedReader;

void _show_buffer(BufferedReader *br)
{
	debug("buffer contents");
	for (size_t i=0; i < br->buffsize; ++i)
		putchar(br->buff[i]);
	putchar('\n');
	debug("end buffer contents");
}
#ifdef DEBUG
#define debug_show_buffer(br) _show_buffer(br)
#else
#define debug_show_buffer(br)
#endif


size_t br_fill(BufferedReader *br, char *keep, size_t len)
{  /* refill the buffer from the underlying stream. Return the number
    * size of the stored text.
    * 
    * - `keep` is a pointer to the beginning of the string to keep.
    * - `len` is the length of the string to keep.
    */
	debug("trying to fill buffer");
	if (len >= br->buffsize) {
		return 0;
	}
	size_t to_read = br->buffsize - len;
	if (len)
	    /* copy the string to keep to the beginning of the buffer */
	    memmove(br->buff, keep, len);
	/* copy data from the file after the saved string. */
	char *dest = br->buff + len;
	size_t bytesread = fread(dest, sizeof(char), to_read, br->stream);
	/* check if stream is exhausted */
	if (bytesread < to_read) {
		br->buffsize = bytesread + len;
		br->end = true;
	}
	/* set current position in the buffer */
	br->at = len;
	debug_show_buffer(br)
	return bytesread + len;
}

BufferedReader *br_new(FILE *stream, size_t buffsize)
{
	BufferedReader *new = malloc(sizeof(BufferedReader));
	if (new == NULL) memoryerror();
	*new = (BufferedReader){stream, malloc(buffsize), buffsize, 0, false};
	if (new->buff == NULL) memoryerror();
	br_fill(new, NULL, 0);
	return new;
}

BufferedReader *br_open(char *name, size_t buffsize)
{
	FILE *stream = fopen(name, "r");
	if (stream == NULL) memoryerror();
	return br_new(stream, buffsize);
}

void br_close(BufferedReader *br)
{
	fclose(br->stream);
	free(br->buff);
	free(br);
}

int br_eof(BufferedReader *br)
{
	if (br->at == br->buffsize) {
	    return feof(br->stream);
	}
	return 0;
}

int br_error(BufferedReader *br)
{
	return ferror(br->stream);
}

int br_getc_keep(BufferedReader *br, char *keep, size_t len, bool *filled)
{
	*filled = false;
	if (br->at == br->buffsize) {
		if (br->end) {
			return EOF;
		}
		if (br_fill(br, keep, len)) {
			*filled = true;
		} else {
			return EOF;
		}
	}
	return br->buff[br->at++];
}

int br_getc(BufferedReader *br, size_t len)
{
	bool filled;
	char *keep = len ? br->buff + (br->buffsize - len) : NULL;
	return br_getc_keep(br, keep, len, &filled);
}

int br_rewind(BufferedReader *br)
{
	if (br->at == 0)
		return 1;
	br->at -= 1;
	return 0;
}

char *br_strptr(BufferedReader *br, size_t length)
{
	if (length > br->at)
		return NULL;
	return br->buff + (br->at - length);
}
