#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "vectors.h"

#define FREE -1
#define DELETED -2

size_t hash(char c)
{
	return (c * 5381);
}
	

typedef struct node Node;
Node *node_new(void);
void node_free(Node *node);

////// hash item impelemetation for a hash table that is part of a trie //////
typedef struct item {
	char c;
	bool val;
	Node *next;
} Item;

Item *item_new(char c)
{
	Item *it = malloc(sizeof(Item));
	it->c = c;
	it->val = false;
	it->next = node_new();
	return it;
}

void item_free(Item *item)
{
	node_free(item->next);
	free(item);
}

////// index for lookup by char //////
typedef struct {
	size_t max;
	short *arr;
} Index;

void index_alloc(Index *idx)
{
	idx->arr = malloc(idx->max * sizeof(short));
	for (int i=0; i < idx->max; ++i) {
		idx->arr[i] = FREE;
	}
}

Index *index_new(size_t init_max)
{
	Index *idx = malloc(sizeof(Index));
	idx->max = init_max;
	index_alloc(idx);
	return idx;
}

void index_free(Index *idx)
{
	free(idx->arr);
	free(idx);
}

void index_grow(Index *idx)
{
	free(idx->arr);
	idx->max *= 2;
	index_alloc(idx);
}

size_t index_mod(Index *index,  size_t i)
{
	return i % (index->max - 1);
}

size_t index_next(Index *index,  size_t i)
{
	return index_mod(index, i + 1);
}

void index_insert(Index *index, char c, size_t position)
{
	size_t i = index_mod(index, hash(c));
	while (index->arr[i] != FREE)
		i = index_next(index, i);
	index->arr[i] = position;
}

////// trie nodes, which are sort of hashtable-y are implement here //////
// items are stuck in a vector type generated from vectors.h
VECTOR(Item *, ItemV, itv);

typedef struct node {
	Index *idx;
	ItemV *items;
} Node;

Node *node_new(void)
{
	Node *node = malloc(sizeof(Node));
	node->idx = index_new(8);
	node->items = itv_new();
	return node;
}

void node_free(Node *node)
{
	index_free(node->idx);
	Item *item;
	EACH(i, item, node->items, itv)
		item_free(item);
	FREE_VECTOR(node->items);
	free(node);
}

typedef struct {Item *item; size_t index;} Lookup;

Lookup node_lookup_char(Node *node, char c)
{
	size_t i = index_mod(node->idx, hash(c));
	short position;
	Item *cur;
	while (true)  {
		if ((position = node->idx->arr[i]) == FREE) {
			Lookup lu = {NULL, i};
			return lu;
		}
		cur = itv_get(node->items, position);
		if (cur->c == c) {
			Lookup lu = {cur, i};
			return lu;
		}
		i = index_next(node->idx, i+1);
	}
}

void node_reindex(Node *node) {
	index_grow(node->idx);
	char c;

	for (int i=0; i < node->items->len; ++i) {
		c = node->items->arr[i]->c;
		index_insert(node->idx, c, i);
	}
}

Item *node_add_char(Node *node, char c)
{
	Lookup lu = node_lookup_char(node, c);
	if (lu.item != NULL) {
		return lu.item;
	}
	ItemV *items = node->items;
	Item *it = item_new(c);
	itv_append(items, it);
	if (node->items->len >= node->idx->max * 1/2) {
		node_reindex(node);
	} else {
		node->idx->arr[lu.index] = items->len - 1;
	}
	return it;
}

Item *node_get_item(Node *node, char c)
{
	return node_lookup_char(node, c).item;
}

int main(int argc, char *argv[])
{
	Node *node = node_new();
	Item *item;
	Lookup lu;
	char *string = argv[0];
	int i, len = strlen(string);
	for (i=0; i < 256; ++i) {
		printf("%c\n", i);
		item = node_add_char(node, i);
	}
	char s[] = "asd";
	for (i=0; i < 4; ++i) {
		lu = node_lookup_char(node, s[i]);
		if (lu.item != NULL) {
		    printf("%c -> %c, %ld\n", s[i], lu.item->c, lu.index);
		}
	}
	for (int j=0; j < node->idx->max; ++j) {
		if ((i = node->idx->arr[j]) != FREE) {
			printf("%c ", itv_get(node->items, i)->c);
		} else {
			printf("0 ");
		}
	}
	puts("");
	for (i=0; i < node->items->len; ++i) {
		item = node->items->arr[i];
		printf("%c\n", item->c);
		node_lookup_char(node, item->c);
	}
}
