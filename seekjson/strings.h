#ifndef STRINGS_H
#define STRINGS_H

typedef struct String_s String;

String *new_string(char *ptr, size_t length);
String *unsafe_string(char *ptr, size_t);
String *make_string_safe(String *s);
void free_string(String *s);
String *cstringtostring(char *cstring);
char *stringtocstring(char *dest, String *s);
String *update_string(String *s, const char *ptr, const size_t length);
bool strings_are_equal(const String a, const String b);

#endif
