#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

void memoryerror(void)
{
	fputs("could not allocate memory\n", stderr);
	exit(1);
}

typedef struct {
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

Key ks_pop(KeyStack *stack)
{
	return stack->a[--(stack->len)];
}

void ks_replace(const KeyStack *stack, const Key key)
{
	stack->a[stack->len-1] = key;
}

Key ks_get_top(const KeyStack *stack)
{
	return stack->a[stack->len-1];
}

typedef struct {
	FILE *stream;
	KeyStack *stack;
	char *buff;
	size_t buffsize;
} Scanner;

Scanner *new_scanner(FILE *stream, size_t buffsize)
{
	Scanner *new = malloc(sizeof(Scanner));
	new->stream = stream;
	new->stack = ks_new(16);
	new->buff = malloc(sizeof(char) * buffsize);
	new->buffsize = buffsize;
}

void free_scanner(Scanner *scanner)
{
	free(scanner->buff);
	ks_free(scanner->stack);
	free(scanner);
}

size_t grow_buffer(Scanner *scanner)
{
	size_t new_size = scanner->buffsize * 2;
	char *buff = realloc(scanner->buff, sizeof(char) * new_size);
	if (buff == NULL)
		memoryerror();
	scanner->buffsize = new_size;
	scanner->buff = buff;
	return new_size;
}

void add_char_to_buffer(Scanner *scanner, size_t idx, char c) {
	if (idx == scanner->buffsize)
		grow_buffer(scanner);
	scanner->buff[idx] = c;
}

typedef enum {
	 json_object,
	 json_array,
	 json_string,
	 json_number,
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
	case EOF:
		return json_eof;
	default:
		return json_malformed;
	}
}

bool in_object(const Scanner *scanner)
{
	Key top = ks_get_top(scanner->stack);
	if (top.key == NULL)
		return false;
	return true;
}

size_t add_string_to_buffer(Scanner *scanner, size_t start)
{
	scanner->buff[start] = '"';
	size_t len;
	int c;
	for (len = start+1 ;(c = fgetc(scanner->stream)) != '"'; ++len) {
		if (c == EOF)
			return 0;
		add_char_to_buffer(scanner, len, c);
		if (c == '\\')
			add_char_to_buffer(scanner, ++len, fgetc(scanner->stream));
	}
	add_char_to_buffer(scanner, len++, c);
	return len;
}

String copy_string(Scanner *scanner)
{
	size_t len = add_string_to_buffer(scanner, 0);
	String out = {len, scanner->buff};
	return out;
}

int eatwhitespace(FILE *stream)
{
	int c;
	while (isspace(c = fgetc(stream)));
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
	Key key = {NULL, 0};
	if (c == 0)
		c = eatwhitespace(scanner->stream);
	if (in_object(scanner)) {
		switch(c) {
		case ',':
			*key.key = copy_string(scanner);
			break;
		case '}':
			ks_pop(scanner->stack);
			return set_next_key(scanner, 0);
		case EOF:
			return check_eof(scanner->stack);
		default:
			return json_malformed;
		}
		c = eatwhitespace(scanner->stream);
		if (c != ':') {
			return json_malformed;
		}
	} else {
		switch(c) {
		case ',':
			key.idx = ks_get_top(scanner->stack).idx + 1;
			break;
		case ']':
			ks_pop(scanner->stack);
			return set_next_key(scanner, 0);
		case EOF:
			return check_eof(scanner->stack);
		default:
			return json_malformed;
		}
	}
	ks_replace(scanner->stack, key);
	return json_object;
}

JsonType scan_next(Scanner *scanner);

JsonType scan_string(Scanner *scanner)
{
	int c;
	while ((c = fgetc(scanner->stream)) != '"') {
		if (c == EOF)
			return json_malformed;
		if (c == '\\')
			fgetc(scanner->stream);
	}
	return set_next_key(scanner, 0);
}

JsonType scan_number(Scanner *scanner)
{
	int c;
	while (isdigit(c = fgetc(scanner->stream)));
	if (c == '.')
		while (isdigit(c = fgetc(scanner->stream)));
	if (c == 'e' || c == 'E') {
		c = fgetc(scanner->stream);
		if (c == '+' || '-')
			c = fgetc(scanner->stream);
		while (isdigit(c = fgetc(scanner->stream)));
	}
	return set_next_key(scanner, c);
}

JsonType scan_object(Scanner *scanner)
{
	String s = {0, ""};
	Key newkey = {&s, 0};
	ks_push(scanner->stack, newkey);
	return set_next_key(scanner, 0);
}

JsonType scan_array(Scanner *scanner)
{
	Key newkey = {NULL, 0};
	ks_push(scanner->stack, newkey);
	return json_array;
}

JsonType scan_next(Scanner *scanner)
{
	int c = eatwhitespace(scanner->stream);
	JsonType jtype = get_type(c);
	switch (jtype) {
	case json_string:
		return scan_string(scanner);
	case json_number:
		return scan_number(scanner);
	case json_object:
		return scan_object(scanner);
	case json_array:
		return scan_array(scanner);
	case json_eof:
		return check_eof(scanner->stack);
	default:
		return json_malformed;
	}
}

// just for testing.
int main(void)
{
	
