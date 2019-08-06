#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "bufferedreader.h"
#include "memoryerror.h"

typedef struct String_s {
	size_t len;
	char *a;
} String;

bool strings_are_equal(const String a, const String b)
{
	if (a.len != b.len)
		return false;
	char *pa = a.a;
	char *pb = b.a;
	for (size_t i=0; i < a.len; ++i) 
		if (*(pa++) != *(pb++))
			return false;
	return true;
}

String *new_string(char *ptr, size_t length)
{
	String *s = malloc(sizeof(String));
	if (s == NULL)
		memoryerror();
	s->len = length;
	s->a = malloc(length);
	if (s->a == NULL)
		memoryerror();
	memcpy(s->a, ptr, length);
	return s;
}

char *stringtocstring(char *dest, String *s)
{
	memcpy(dest, s->a, s->len);
	dest[s->len] = '\0';
	return dest;
}

void free_string(String *s)
{
	if (s == NULL)
		return;
	free(s->a);
	free(s);
}

void update_string(String *s, const char *ptr, const size_t length)
{
	if (s->len < length) {
		s->a = realloc(s->a, length);
		if (s->a == NULL)
			memoryerror();
	}
	s->len = length;
	memcpy(s->a, ptr, length);
}

typedef struct {
	String *key;
	size_t idx;
} Key;

typedef struct {
	size_t max;
	size_t len;
	Key *a;
} KeyStack;

KeyStack *ks_new(const size_t buffsize)
{
	Key *a = malloc(sizeof(Key) * buffsize);
	if (a == NULL) memoryerror();
	KeyStack *stack = malloc(sizeof(KeyStack));
	if (stack == NULL) memoryerror();
	KeyStack tempkey = {buffsize, 0, a};
	*stack = tempkey;
	return stack;
}

void ks_free(KeyStack *stack)
{
	for (size_t i = 0; i < stack->len; ++i)
		free_string(stack->a[i].key);
	free(stack->a);
	free(stack);
}

size_t ks_grow(KeyStack *stack)
{
	size_t max = stack->max * 2;
	Key *a = realloc(stack->a, sizeof(Key) * max);
	if (a == NULL) memoryerror();
	stack->max = max;
	stack->a = a;
	return max;
}

void ks_push(KeyStack *stack, const Key key)
{
	if (stack->len == stack->max)
		ks_grow(stack);
	stack->a[stack->len++] = key;
}

Key *ks_get_top(const KeyStack *stack)
{
	return stack->a + (stack->len-1);
}

Key ks_pop(KeyStack *stack)
{
	Key *k = ks_get_top(stack);
	// problem could be here.
	free_string(k->key);
	return stack->a[--(stack->len)];
}

void ks_replace(const KeyStack *stack, const Key key)
{
	stack->a[stack->len-1] = key;
}

typedef struct {
	BufferedReader *stream;
	KeyStack *stack;
} Scanner;

Scanner *new_scanner(FILE *stream, size_t buffsize)
{
	Scanner *new = malloc(sizeof(Scanner));
	if (new == NULL)
		memoryerror();
	new->stream = br_new(stream, buffsize);
	new->stack = ks_new(16);
	return new;
}

void free_scanner(Scanner *scanner)
{
	br_close(scanner->stream);
	ks_free(scanner->stack);
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

size_t add_jstring_to_buffer(BufferedReader *stream, const size_t keep)
{
	size_t len = keep;
	int c;
	for (len += 1 ;(c = br_getc(stream, len)) != '"'; ++len) {
		if (c == EOF)
			return 0;
		if (c == '\\')
			br_getc(stream, ++len);
	}
	return ++len;
}

void copy_jstring(String *s, BufferedReader *stream)
{
	size_t len = add_jstring_to_buffer(stream, 0);
	update_string(s, br_strptr(stream, len), len);
}

int eatwhitespace(BufferedReader *stream)
{
	int c;
	while (isspace(c = br_getc(stream, 0)));
	return c;
}

JsonType check_eof(KeyStack *stack)
{
	if (stack->len == 0)
		return json_eof;
	return json_malformed;
}

JsonType set_next_key(Scanner *scanner, int c)
{
	Key *key = ks_get_top(scanner->stack);
	if (key->key != NULL) {
		// in object
		switch(c) {
		case ',':
			c = eatwhitespace(scanner->stream);
			copy_jstring(key->key, scanner->stream);
			c = eatwhitespace(scanner->stream);
			if (c != ':')
				return json_malformed;
			return json_object;
		case '}':
			ks_pop(scanner->stack);
			return json_object;
		case EOF:
			return check_eof(scanner->stack);
		default:
			return json_malformed;
		}
	} else {
		//in array
		switch(c) {
		case ',':
			key->idx += 1;
			return json_array;
		case ']':
			ks_pop(scanner->stack);
			return set_next_key(scanner, 0);
		case EOF:
			return check_eof(scanner->stack);
		default:
			return json_malformed;
		}
	}
}

JsonType scan_string(Scanner *scanner)
{
	int c;
	while ((c = br_getc(scanner->stream, 0)) != '"') {
		if (c == EOF)
			return json_malformed;
		if (c == '\\')
			br_getc(scanner->stream, 0);
	}
	return set_next_key(scanner, 0);
}

JsonType scan_number(Scanner *scanner)
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
	return set_next_key(scanner, c);
}

JsonType scan_object(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	if (c == '}')
		return set_next_key(scanner, c);
	if (c != '"')
		return json_malformed;
	br_rewind(scanner->stream);
	String *s = new_string("1234567890123456789012", 32);
	Key newkey = {s, 0};
	ks_push(scanner->stack, newkey);
	return set_next_key(scanner, ',');
}

JsonType scan_array(Scanner *scanner)
{
	Key newkey = {NULL, 0};
	ks_push(scanner->stack, newkey);
	return json_array;
}

JsonType scan_null(Scanner *scanner)
{
	if (br_getc(scanner->stream, 0) != 'u')
		return json_malformed;
	for (size_t i = 0; i < 2; ++i)
		if (br_getc(scanner->stream, 0) != 'l') {
			return json_malformed;
		}
	return set_next_key(scanner, eatwhitespace(scanner->stream));
}
		

JsonType scan_next(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	switch (get_type(c)) {
	case json_string:
		return scan_string(scanner);
	case json_number:
		return scan_number(scanner);
	case json_object:
		return scan_object(scanner);
	case json_array:
		return scan_array(scanner);
	case json_null:
		return scan_null(scanner);
	case json_eof:
		return check_eof(scanner->stack);
	default:
		return json_malformed;
	}
}

int main(void) {
	Scanner *scanner = new_scanner(stdin, 64);
	JsonType jtype;
	Key *key;
	char out[72];
	while ((jtype = scan_next(scanner)) != json_eof && jtype != json_malformed) {
		key = ks_get_top(scanner->stack);
		if (key->key != NULL) {
			puts(stringtocstring(out, key->key));
		} else {
			printf("%ld\n", key->idx);
		}
	}
	if (jtype == json_malformed)
		puts("malformed");
}

