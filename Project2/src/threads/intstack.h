#ifndef _int_stack_
#define _int_stack_

#include <stdbool.h>
#include <assert.h>

typedef struct{
	int elems[2000];
	int size;
} stack;

void stack_init(stack* st){
	st->size = 0;
}

void stack_dispose(stack* st){

}

int stack_peek(stack* st){
	assert(st->size>=0);

	return st->elems[st->size-1];
}

void stack_pop(stack* st){
	assert(st->size>=0);
	st->size--;
}

void stack_push(stack* st, int elem){
	
	st->elems[st->size] = elem;
	st->size++;
}

bool stack_empty(stack* st){
	return st->size==0;
}

#endif

/*
// testing code for stack

#include <stdio.h>
#include "stack.h"
int main(){

	stack st;

	stack_init(&st);

	int a=1;
	int b=2;
	int c=3;
	int d=4;
	int e=5;
	int f=6;
	int g=6;
	int h=7;

	stack_push(&st, a);

	stack_push(&st, b);

	stack_push(&st, c);

	stack_push(&st, d);

	printf("last elem is %d\n", stack_peek(&st));

	stack_pop(&st);
	printf("last elem is %d\n", stack_peek(&st));

	stack_pop(&st);
	printf("last elem is %d\n", stack_peek(&st));

	stack_pop(&st);

	stack_pop(&st);

	printf("last elem is %d\n", stack_empty(&st));



	return 0;
}


*/