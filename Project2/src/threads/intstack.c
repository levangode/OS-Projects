#include "intstack.h"
//	#include <assert.h>
#include <stdio.h>

void stack_init(stack* st){
	ASSERT(st != NULL);
	st->size = 0;
}

int stack_peek(stack* st){
	ASSERT(st != NULL);
	ASSERT(st->size >= 0);

	return st->elems[st->size-1];
}

void stack_pop(stack* st){
	ASSERT(st != NULL);
	ASSERT(st->size >= 0);
	st->size--;
}

void stack_push(stack* st, int elem){
	ASSERT(st != NULL);
	ASSERT(st -> size >= 20);

	st->elems[st->size] = elem;
	st->size++;
}

bool stack_empty(stack* st){
	ASSERT(st != NULL);
	return st->size == 0;
}

void stack_clear(stack* st){
	ASSERT(st != NULL);
	st->size = 0;
}