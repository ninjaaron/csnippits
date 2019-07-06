// a macro to create arbitrary typed dynamic lists
#include <stdlib.h>
#include <stdbool.h>

#define VECTOR(TYPE, NAME, PREFIX)					\
	typedef struct _ ## NAME {					\
		size_t len;						\
		size_t max;						\
		TYPE *arr;						\
	} NAME;								\
									\
	NAME *PREFIX ## _new(void)					\
	{								\
		NAME *new = malloc(sizeof(NAME));			\
		new->arr = malloc(sizeof(TYPE) * 2);			\
		new->len = 0;						\
		new->max = 2;						\
		return new;						\
	}								\
									\
	bool PREFIX ## _grow(NAME *vector)				\
	{								\
		vector->max *= 2;					\
		TYPE *tmp;						\
		tmp = realloc(vector->arr, sizeof(TYPE) * vector->max);	\
		if (tmp == NULL)					\
			return true;					\
		vector->arr = tmp;					\
		return false;						\
	}								\
									\
	bool PREFIX ## _prune(NAME *vector)				\
	{								\
		size_t max = vector->max;				\
		while ((max /= 2) >= vector->len)			\
			;						\
		max *= 2;						\
		if (max == vector->max) {				\
			return false;					\
		}							\
		vector->max = max;					\
		TYPE *tmp;						\
		tmp = realloc(vector->arr, sizeof(TYPE) * vector->max);	\
		if (tmp == NULL)					\
			return true;					\
		vector->arr = tmp;					\
		return false;						\
	}								\
									\
	bool PREFIX ## _append(NAME *vector, TYPE val)			\
	{								\
		bool err = false;					\
		if (vector->len == vector->max) {			\
			err = PREFIX ## _grow(vector);			\
		}							\
		vector->arr[vector->len++] = val;			\
		return err;						\
	}								\
									\
	bool PREFIX ## _extend(NAME *target, NAME *source)		\
	{								\
		TYPE *tmp;						\
		TYPE *sarr = source->arr;				\
		size_t len = target->len + source->len;			\
		size_t slen = source->len;				\
		while (target->max < len) {				\
			target->max *= 2;				\
		}							\
		tmp = realloc(target->arr, sizeof(TYPE) * target->max);	\
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
	TYPE PREFIX ## _pop(NAME *vector)				\
	{								\
		if (vector->len == 0) {					\
			fputs(#PREFIX "_pop called on empty vector",	\
			      stderr);					\
		}							\
		return vector->arr[--vector->len];			\
	}								\
									\
	TYPE PREFIX ## _del(NAME *vector, size_t idx)			\
	{								\
		--vector->len;						\
		TYPE val = vector->arr[idx];				\
		TYPE *end = vector->arr + vector->len - 1;		\
		TYPE *cur = vector->arr + idx;				\
		do {							\
			*cur = cur[1];					\
		} while (++cur < end);					\
		return val;						\
	}								\
									\
	TYPE PREFIX ## _get(NAME *vector, size_t idx)			\
	{								\
		if (idx >= vector->len) {				\
			fputs(#PREFIX "_get: no such index.\n",		\
			      stderr);					\
		} else if (idx < 0) {					\
			idx = vector->len + idx;			\
		}							\
		return vector->arr[idx];				\
	}								\
									\
	size_t PREFIX ## _next(NAME *vector, size_t idx, TYPE *ptr)	\
	{								\
		if (idx >= vector->len) {				\
			return 0;					\
		}							\
		*ptr = vector->arr[idx];				\
		return ++idx;						\
	}								\
									\
	void PREFIX ## _clear(NAME *vector)				\
	{								\
		free(vector->arr);					\
		vector->arr = malloc(sizeof(TYPE) * 2);			\
		vector->len = 0;					\
		vector->max = 2;					\
	}								\


#define FREE_VECTOR(vectorptr) free(vectorptr->arr); free(vectorptr)

#define EACH(counter, var, vectorptr, prefix)				\
	for (counter = 0;(counter = prefix ## _next(vectorptr, counter, &var));)
