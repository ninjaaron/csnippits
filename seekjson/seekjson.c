#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
	size_t len;
	char * a;
} String;

typedef struct {
	String * key;
	size_t idx;
} ValueAddress;

typedef struct {
	size_t len;
	size_t start;
} ValueLocation;

typedef struct {
	size_t max;
	size_t len;
	ValueAddress * a;
} AddressStack;

int as_grow(AddressStack * stack)
{
	size_t newmax = stack->max * 2;
	ValueAddress * new_a =
		realloc(stack->a, sizeof(ValueAddress) * newmax);
	if (new_a == NULL)
		return 1;
	free(stack->a);
	stack->max = newmax;
	stack->a = new_a;
	return 0;
}

int as_push(AddressStack * stack, ValueAddress addr)
{
	// grow stack if necessary
	if (stack->len == stack->max)
		if (int err = as_grow(stack))
			return err;
	stack->a[stack->len] = addr;
	++stack->len;
	return 0;
}

ValueAddress as_pop(AddressStack * stack)
{
	size_t len = --stack->len;
	return stack->a[len];
}
