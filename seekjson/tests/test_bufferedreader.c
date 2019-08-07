#include "bufferedreader.c"

int main(void) {
	BufferedReader *br = br_open("test_bufferedreader.c", 8);
	char string[9];
	string[8] = '\0';
	memccpy(string, br->buff, 1, 8);
	puts(string);
	br_fill(br, 3);
	memccpy(string, br->buff, 1, 8);
	puts(string);
	br_fill(br, 0);
	memccpy(string, br->buff, 1, 8);
	puts(string);
	int c;
	while ((c = br_getc(br, 0)) != EOF)
		putchar(c);
	
	br_close(br);
}
