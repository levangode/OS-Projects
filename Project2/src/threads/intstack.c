#include "intstack.h"

void stack_init(stack* st){
	st->size = 0;
}

void stack_dispose(stack* st UNUSED){

}

int stack_peek(stack* st){
	ASSERT(st->size>=0);

	return st->elems[st->size-1];
}

void stack_pop(stack* st){
	ASSERT(st->size>=0);
	st->size--;
}

void stack_push(stack* st, int elem){
	
	st->elems[st->size] = elem;
	st->size++;
}

bool stack_empty(stack* st){
	return st->size==0;
}