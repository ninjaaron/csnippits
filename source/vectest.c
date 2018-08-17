#include <stdio.h>
#include <assert.h>
#include "vectors.h"

VECTOR(char *, IntV, iv);

int main(int argc, char *argv[])
{
	int i;
	IntV *my_vec = iv_new();
	for (i=0; i < argc; ++i) {
		iv_append(my_vec, argv[i]);
	}
	char *word;
	EACH(i, word, my_vec, iv) {
		puts(word);
	}
	while(my_vec->len) {
		puts(iv_pop(my_vec));
	}
}
	
