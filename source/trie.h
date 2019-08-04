// This file must be processed with longCmacros.py

#include <stdint.h>
#include <stdbool.h>

enum MapType {
	full,
	tiny,
	tree,
};

#define TRIE(Name, T, pfx, empty)

typedef struct {
	MapType t;
	void * map;
} Name;

typedef struct {
	char c;
	T val;
	Name children;
} NameNode;

#enddefine
