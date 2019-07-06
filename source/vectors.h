// a macro to create arbitrary typed dynamic lists
#include <stdlib.h>
#include <stdbool.h>

#define VECTOR(type, name, prefix)					\
	typedef struct _ ## name {					\
		size_t len;						\
		size_t max;						\
		type *arr;						\
	} name;								\
									\
	name *prefix ## _new(void)					\
	{								\
		name *new = malloc(sizeof(name));			\
		new->arr = malloc(sizeof(type) * 2);			\
		new->len = 0;						\
		new->max = 2;						\
		return new;						\
	}								\
									\
	bool prefix ## _grow(name *vector)				\
	{								\
		vector->max *= 2;					\
		type *tmp;						\
		tmp = realloc(vector->arr, sizeof(type) * vector->max);	\
		if (tmp == NULL)					\
			return true;					\
		vector->arr = tmp;					\
		return false;						\
	}								\
									\
	bool prefix ## _prune(name *vector)				\
	{								\
		size_t max = vector->max;				\
		while ((max /= 2) >= vector->len)			\
			;						\
		max *= 2;						\
		if (max == vector->max) {				\
			return false;					\
		}							\
		vector->max = max;					\
		type *tmp;						\
		tmp = realloc(vector->arr, sizeof(type) * vector->max);	\
		if (tmp == NULL)					\
			return true;					\
		vector->arr = tmp;					\
		return false;						\
	}								\
									\
	bool prefix ## _append(name *vector, type val)			\
	{								\
		bool err = false;					\
		if (vector->len == vector->max) {			\
			err = prefix ## _grow(vector);			\
		}							\
		vector->arr[vector->len++] = val;			\
		return err;						\
	}								\
									\
	bool prefix ## _extend(name *target, name *source)		\
	{								\
		type *tmp;						\
		type *sarr = source->arr;				\
		size_t len = target->len + source->len;			\
		size_t slen = source->len;				\
		while (target->max < len) {				\
			target->max *= 2;				\
		}							\
		tmp = realloc(target->arr, sizeof(type) * target->max);	\
		if (tmp == NULL)					\
			return true;					\
		target->arr = tmp;					\
		tmp = target->arr+len;					\
		for (size_t i=0; i < slen; ++i) {			\
			*tmp++ = *sarr++;				\
		}							\
		return false;						\
	}								\
									\
	type prefix ## _pop(name *vector)				\
	{								\
		if (vector->len == 0) {					\
			fputs(#prefix "_pop called on empty vector",	\
			      stderr);					\
		}							\
		return vector->arr[--vector->len];			\
	}								\
									\
	type prefix ## _del(name *vector, size_t idx)			\
	{								\
		--vector->len;						\
		type val = vector->arr[idx];				\
		type *end = vector->arr + vector->len - 1;		\
		type *cur = vector->arr + idx;				\
		do {							\
			*cur = cur[1];					\
		} while (++cur < end);					\
		return val;						\
	}								\
									\
	type prefix ## _get(name *vector, size_t idx)			\
	{								\
		if (idx >= vector->len) {				\
			fputs(#prefix "_get: no such index.\n",		\
			      stderr);					\
		} else if (idx < 0) {					\
			idx = vector->len + idx;			\
		}							\
		return vector->arr[idx];				\
	}								\
									\
	size_t prefix ## _next(name *vector, size_t idx, type *ptr)	\
	{								\
		if (idx >= vector->len) {				\
			return 0;					\
		}							\
		*ptr = vector->arr[idx];				\
		return ++idx;						\
	}								\
									\
	void prefix ## _clear(name *vector)				\
	{								\
		free(vector->arr);					\
		vector->arr = malloc(sizeof(type) * 2);			\
		vector->len = 0;					\
		vector->max = 2;					\
	}								\


#define FREE_VECTOR(vectorptr) free(vectorptr->arr); free(vectorptr)

#define EACH(counter, var, vectorptr, prefix)				\
	for (counter = 0;(counter = prefix ## _next(vectorptr, counter, &var));)
