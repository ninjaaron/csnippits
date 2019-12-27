#ifndef BUFFERED_READER
#define BUFFERED_READER

#if defined(__cplusplus)
extern "C" {
#endif
 
	#include <stddef.h>
	/* A horrifying ball of state. */
	typedef struct BufferedReader_s BufferedReader;

	/* construct a new buffered stream reader */
	BufferedReader *br_new(FILE *stream, size_t buffsize);
	/* close and free buffered reader */
	void br_close(BufferedReader *br);
	/* open a filename `name` and construct a new buffered stream reader */
	BufferedReader *br_open(char *name, size_t buffsize);
	/* see if the end of file has been reached */
	int br_eof(BufferedReader *br);
	/* check the underlying stream for errors */
	int br_error(BufferedReader *br);
	/* get the next character. Ensure the previous `len` characters
	   are kept */
	int br_getc(BufferedReader *br, size_t len);
	/* get the next character. Ensure the string at `keep` with a
	   length of `len` is retained in the buffer. `filled` is set
	   to `true` if the buffer was refilled, otherwise set to
	   `false`. */
	int br_getc_keep(BufferedReader *br, char *keep, size_t len, bool *filled);
	/* rewind the buffer by one character. Return 1 if the buffer
	   is at the beginning and cannot be rewound, otherwise return
	   0. */
	void br_rewind(BufferedReader *br);
	/* return a pointer starting `length` chars from the end of
	   the buffer */
	char *br_strptr(BufferedReader *br, size_t length);
 
#if defined(__cplusplus)
} /* extern "C" */
#endif
 
#endif
