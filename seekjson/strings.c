#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "memoryerror.h"

typedef struct String_s {
	size_t len;
	char *a;
	bool on_heap;
} String;

String *new_string(char *ptr, size_t length)
{
	String *s = malloc(sizeof(String));
	if (s == NULL) memoryerror();
	s->len = length;
	s->a = malloc(length);
	s->on_heap = true;
	if (s->a == NULL) memoryerror();
	memcpy(s->a, ptr, length);
	return s;
}

String *unsafe_string(char *ptr, size_t length)
{
	String *s = malloc(sizeof(String));
	if (s == NULL) memoryerror();
	s->len = length;
	s->a = ptr;
	s->on_heap = false;
	return s;
}

String *make_string_safe(String *s)
{
	if (s->on_heap)
		return s;
	char *a = s->a;
	s->a = malloc(s->len);
	s->on_heap = true;
	if (s->a == NULL) memoryerror();
	memcpy(s->a, a, s->len);
	return s;
}

void free_string(String *s)
{
	if (s == NULL)
		return;
	if (s->on_heap)
		free(s->a);
	free(s);
}

String *cstringtostring(char *cstring)
{
	return new_string(cstring, strlen(cstring)-1);
}

char *stringtocstring(char *dest, String *s)
{
	memcpy(dest, s->a, s->len);
	dest[s->len] = '\0';
	return dest;
}

String *update_string(String *s, char *ptr, const size_t length)
{
	if (! s->on_heap) {
		s->a = ptr;
		s->len = length;
		return s;
	}
	if (s->len < length) {
		s->a = realloc(s->a, length);
		if (s->a == NULL) memoryerror();
	}
	s->len = length;
	memcpy(s->a, ptr, length);
	return s;
}

// all strings being equal...
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
