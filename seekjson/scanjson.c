#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "bufferedreader.h"
#include "strings.h"

#define DEBUG
#include "memoryerror.h"

// define some types
typedef enum {
	 json_malformed = 0,
	 json_object = '{',
	 json_array = '[',
	 json_string = '"',
	 json_number = '0',
	 json_null = 'n',
	 json_eof = 256
} JsonType;

typedef enum {
	j_none,
	j_follower,
} CharClasses;

CharClasses charclasses[266] = {
	[','] = j_follower,
	['}'] = j_follower,
	[']'] = j_follower,
};

JsonType get_type(const int c)
{
	if (isdigit(c))
		return json_number;
	switch(c) {
	case '-' :
		return json_number;
	case EOF:
		return json_eof;
	default:
		return c;
	}
}


// BufferedReader functions.
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

int eatifspace(BufferedReader *stream, int c)
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

// ObjStack implementation
typedef struct {
	size_t max;
	size_t len;
	ssize_t *idxs;
} ObjStack;

ObjStack *os_new(const size_t buffsize)
{
	ObjStack *stack = malloc(sizeof(ObjStack));
	if (stack == NULL) memoryerror();
	*stack = (ObjStack){buffsize, 0, malloc(sizeof(ssize_t) * buffsize)};
	if (stack->idxs == NULL) memoryerror();
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

JsonType check_eof(ObjStack *stack)
{
	if (stack->len == 0)
		return json_eof;
	return json_malformed;
}

// Scanner implementation
typedef struct {
	BufferedReader *stream;
	ObjStack *stack;
	String *key;
} Scanner;

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

JsonType eat_string(Scanner *scanner)
{
	int c;
	while ((c = br_getc(scanner->stream, 0)) != '"') {
		if (c == EOF)
			return json_malformed;
		if (c == '\\')
			br_getc(scanner->stream, 0);
	}
	return json_string;
}

// enter_object and enter_array assume the next in the scanner will be
// an object or an array respectively. From there, you can iterate over
// keys/indices inside that object to see if there is anything
// interesting, either passing or reading (or entering).
JsonType enter_object(Scanner *scanner)
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

JsonType enter_array(Scanner *scanner)
{
	os_push(scanner->stack, 0);
	return json_array;
}

// passing functions: used to scan past objects without reading. Each
// sets the the key for the next item in the array or object you're in.
JsonType pass_string(Scanner *scanner)
{
	JsonType t = eat_string(scanner);
	if (t == json_malformed)
		return t;
	int c;
	c = eatwhitespace(scanner->stream);
	return set_next_key(scanner, c);
}

JsonType pass_number(Scanner *scanner)
{
	int c;
	while (!isspace(c = br_getc(scanner->stream, 0))
	       && charclasses[c] != j_follower && c != EOF
	       );
	c = eatifspace(scanner->stream, c);
	return set_next_key(scanner, c);
}

JsonType pass_array(Scanner *scanner);
JsonType pass_object(Scanner *scanner);

JsonType pass_object_inner(Scanner *scanner, int stopchar)
{
	int c;
	while ((c = br_getc(scanner->stream, 0)) != stopchar) {
		switch (c) {
		case '"':
			eat_string(scanner);
			break;
		case '{':
			pass_object(scanner);
			break;
		case '[':
			pass_array(scanner);
			break;
		case EOF:
			return json_malformed;
		}
	}
	c = eatwhitespace(scanner->stream);
	return set_next_key(scanner, c);
}
	
JsonType pass_object(Scanner *scanner)
{
	return pass_object_inner(scanner, '}');
}

JsonType pass_array(Scanner *scanner)
{
	return pass_object_inner(scanner, ']');
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

ssize_t tell_index(Scanner *scanner)
{
	return os_last(scanner->stack);
}

typedef JsonType (*PassFunc)(Scanner *);
PassFunc passfuncs[266] = {
	[json_object] = pass_object,
	[json_array] = pass_array,
	[json_string] = pass_string,
	[json_null] = pass_null,
};

JsonType pass_next(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	if (c == EOF)
		return check_eof(scanner->stack);
	PassFunc pass_func = passfuncs[c];
	if (! pass_func ==  0)
		return pass_func(scanner);
	switch(get_type(c)) {
	case json_number:
		return pass_number(scanner);
	case json_eof:
	default:
		return json_malformed;
	}
}

// completely and utterly broken. no number handling. no whitespace handling.
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


Scanner *new_scanner(FILE *stream, size_t buffsize)
{
	Scanner *new = malloc(sizeof(Scanner));
	if (new == NULL) memoryerror();
	*new = (Scanner){
		br_new(stream, buffsize), os_new(16), unsafe_string("0", 1)};
	return new;
}

void free_scanner(Scanner *scanner)
{
	br_close(scanner->stream);
	os_free(scanner->stack);
	free(scanner);
}

int main(void) {
	Scanner *scanner = new_scanner(stdin, 64);
	br_getc(scanner->stream, 0);
	JsonType jtype = enter_object(scanner);
	char out[72];
	puts(stringtocstring(out, scanner->key));
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
