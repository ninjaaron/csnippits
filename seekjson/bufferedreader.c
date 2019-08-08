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
	size_t at;
	bool end;
} BufferedReader;

size_t br_fill(BufferedReader *br, char *keep, size_t len)
{
	debug("trying to fill buffer");
	if (len >= br->buffsize) {
		return 0;
	}
	size_t to_read = br->buffsize - len;
	if (len)
	    memmove(br->buff, keep, len);
	char *dest = br->buff + len;
	size_t bytesread = fread(dest, sizeof(char), to_read, br->stream);
	if (bytesread < to_read) {
		br->buffsize = bytesread + len;
		br->end = true;
	}
	br->at = len;
	#ifdef DEBUG
	debug("buffer contents");
	for (size_t i=0; i < br->buffsize; ++i)
		putchar(br->buff[i]);
	putchar('\n');
	debug("end buffer contents");
	#endif
	return bytesread + len;
}

BufferedReader *br_new(FILE *stream, size_t buffsize)
{
	BufferedReader *new = malloc(sizeof(BufferedReader));
	if (new == NULL) memoryerror();
	new->stream = stream;
	new->buff = malloc(sizeof(char) * buffsize);
	if (new->buff == NULL) memoryerror();
	new->buffsize = buffsize;
	new->end = false;
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
	return feof(br->stream);
}

int br_error(BufferedReader *br)
{
	return ferror(br->stream);
}

int br_getc_keep(BufferedReader *br, char *keep, size_t len, bool *filled)
{
	*filled =false;
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
	bool filled = false;
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
