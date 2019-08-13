#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "bufferedreader.h"
#include "strings.h"

#define DEBUG
#include "memoryerror.h"

typedef struct {
	size_t max;
	size_t len;
	ssize_t *idxs;
} ObjStack;

ObjStack *os_new(const size_t buffsize)
{
	ssize_t *a = malloc(sizeof(ssize_t) * buffsize);
	if (a == NULL) memoryerror();
	ObjStack *stack = malloc(sizeof(ObjStack));
	if (stack == NULL) memoryerror();
	stack->max = buffsize;
	stack->len = 0;
	stack->idxs = a;
	return stack;
}

#define os_last(stack) stack->idxs[(stack->len)-1]

void os_free(ObjStack *stack)
{
	free(stack->idxs);
	free(stack);
}

size_t os_grow(ObjStack *stack)
{
	size_t max = stack->max * 2;
	ssize_t *a = realloc(stack->idxs, sizeof(ssize_t) * max);
	if (a == NULL) memoryerror();
	stack->max = max;
	stack->idxs = a;
	return max;
}

void os_push(ObjStack *stack, const ssize_t idx)
{
	if (stack->len == stack->max)
		os_grow(stack);
	stack->idxs[stack->len++] = idx;
}

bool os_pop(ObjStack *stack)
{
	return stack->idxs[--(stack->len)];
}

void os_replace(const ObjStack *stack, const ssize_t idx)
{
	os_last(stack) = idx;
}

bool os_in_object(const ObjStack *stack) {
	return os_last(stack) == -1 ? true : false;
}

void os_index_incr(ObjStack *stack)
{
	os_last(stack) += 1; 
}


typedef struct {
	BufferedReader *stream;
	ObjStack *stack;
	String *key;
} Scanner;

Scanner *new_scanner(FILE *stream, size_t buffsize)
{
	Scanner *new = malloc(sizeof(Scanner));
	if (new == NULL) memoryerror();
	new->stream = br_new(stream, buffsize);
	new->stack = os_new(16);
	new->key = unsafe_string("0", 1);
	return new;
}

void free_scanner(Scanner *scanner)
{
	br_close(scanner->stream);
	os_free(scanner->stack);
	free(scanner);
}

typedef enum {
	 json_object,
	 json_array,
	 json_string,
	 json_number,
	 json_null,
	 json_malformed,
	 json_eof
} JsonType;

JsonType get_type(const int c)
{
	if (isdigit(c))
		return json_number;
	switch(c) {
	case '-' :
		return json_number;
	case '"':
		return json_string;
	case '[':
		return json_array;
	case '{':
		return json_object;
	case 'n':
		return json_null;
	case EOF:
		return json_eof;
	default:
		return json_malformed;
	}
}

size_t add_jstring_to_buffer(BufferedReader *stream, size_t keep)
{
	size_t len;
	int c;
	for (len = 1 ;(c = br_getc(stream, keep+len)) != '"'; ++len) {
		if (c == EOF)
			return --len;
		if (c == '\\')
			br_getc(stream, ++len);
	}
	return ++len;
}

int eatwhitespace(BufferedReader *stream)
{
	int c;
	while (isspace(c = br_getc(stream, 0)));
	return c;
}

int eatthisspace(BufferedReader *stream, int c)
{
	while (isspace(c))
		c = br_getc(stream, 0);
	return c;
}

int getc_keep(BufferedReader *stream, String *s)
{
	char *ptr = get_string_ptr(s);
	size_t len = get_string_len(s);
	bool update = false;
	int c = br_getc_keep(stream, ptr, len, &update);
	if (update)
		update_string(s, br_strptr(stream, len+1), len);
	return c;
}

void set_jstring(String *s, BufferedReader *stream)
{
	size_t len = add_jstring_to_buffer(stream, 0);
	update_string(s, br_strptr(stream, len), len);
}

JsonType check_eof(ObjStack *stack)
{
	if (stack->len == 0)
		return json_eof;
	return json_malformed;
}

JsonType set_next_key(Scanner *scanner, int c)
{
	String *key = scanner->key;
	BufferedReader *stream = scanner->stream;
	ObjStack *stack = scanner->stack;
	if (os_in_object(stack)) {
		switch(c) {
		case ',':
			c = eatwhitespace(stream);
			if (c != '"')
				return json_malformed;
			set_jstring(key, stream);
			while (isspace(c = getc_keep(stream, key)));
			if (c != ':')
				return json_malformed;
			return json_object;
		case '}':
			os_pop(stack);
			c = eatwhitespace(stream);
			return set_next_key(scanner, c);
		case EOF:
			return check_eof(stack);
		default:
			return json_malformed;
		}
	} else {
		//in array
		switch(c) {
		case ',':
			os_index_incr(stack);
			return json_array;
		case ']':
			os_pop(stack);
			c = eatwhitespace(scanner->stream);
			return set_next_key(scanner, c);
		case EOF:
			return check_eof(stack);
		default:
			return json_malformed;
		}
	}
}

JsonType pass_string(Scanner *scanner)
{
	int c;
	while ((c = br_getc(scanner->stream, 0)) != '"') {
		if (c == EOF)
			return json_malformed;
		if (c == '\\')
			br_getc(scanner->stream, 0);
	}
	c = eatwhitespace(scanner->stream);
	return set_next_key(scanner, c);
}

JsonType pass_number(Scanner *scanner)
{
	int c;
	while (isdigit(c = br_getc(scanner->stream, 0)));
	if (c == '.')
		while (isdigit(c = br_getc(scanner->stream, 0)));
	if (c == 'e' || c == 'E') {
		c = br_getc(scanner->stream, 0);
		if (c == '+' || '-')
			c = br_getc(scanner->stream, 0);
		while (isdigit(c = br_getc(scanner->stream, 0)));
	}
	c = eatthisspace(scanner->stream, c);
	return set_next_key(scanner, c);
}

JsonType pass_object(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	if (c == '}')
		return set_next_key(scanner, c);
	if (c != '"')
		return json_malformed;
	br_rewind(scanner->stream);
	os_push(scanner->stack, -1);
	return set_next_key(scanner, ',');
}

JsonType pass_array(Scanner *scanner)
{
	os_push(scanner->stack, 0);
	return json_array;
}

JsonType pass_null(Scanner *scanner)
{
	if (br_getc(scanner->stream, 0) != 'u')
		return json_malformed;
	for (size_t i = 0; i < 2; ++i)
		if (br_getc(scanner->stream, 0) != 'l') {
			return json_malformed;
		}
	return set_next_key(scanner, eatwhitespace(scanner->stream));
}
		

JsonType pass_next(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	switch (get_type(c)) {
	case json_string:
		return pass_string(scanner);
	case json_number:
		return pass_number(scanner);
	case json_object:
		return pass_object(scanner);
	case json_array:
		return pass_array(scanner);
	case json_null:
		return pass_null(scanner);
	case json_eof:
		return check_eof(scanner->stack);
	default:
		return json_malformed;
	}
}

String *read_next(Scanner *scanner, String *s)
{
	ObjStack *stack = scanner->stack;
	BufferedReader *stream = scanner->stream;
	size_t stack_base = stack->len;
	int c = eatwhitespace(stream);
	size_t len = 1;
	do {
		switch (c) {
		case '"':
			len += add_jstring_to_buffer(stream, len);
			break;
		case '{':
			os_push(stack, -1);
			break;
		case '[':
			os_push(stack, 0);
			break;
		case '}':
			if (!os_in_object(stack))
				return NULL;
			os_pop(stack);
			break;
		case ']':
			if (os_in_object(stack))
				return NULL;
			os_pop(stack);
			break;
		case json_eof:
			return NULL;
		}
		c = br_getc(stream, len);
		++len;
	} while (stack->len > stack_base);
	br_rewind(stream);
	return update_string(s, br_strptr(stream, len), len);
}

ssize_t tell_index(Scanner *scanner)
{
	return os_last(scanner->stack);
}

int main(void) {
	Scanner *scanner = new_scanner(stdin, 64);
	JsonType jtype;
	char out[72];
	while ((jtype = pass_next(scanner)) != json_eof && jtype != json_malformed) {
		if (os_in_object(scanner->stack)) {
			puts(stringtocstring(out, scanner->key));
		} else {
			printf("%ld\n", tell_index(scanner));
		}
	}
	if (jtype == json_malformed)
		puts("malformed");
}
