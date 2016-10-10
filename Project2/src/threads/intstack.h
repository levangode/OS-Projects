#ifndef _int_stack_
#define _int_stack_

#include <stdbool.h>

typedef struct{
	int elems[2000];
	int size;
} stack;

void stack_init(stack* st);

int stack_peek(stack* st);

void stack_pop(stack* st);

void stack_push(stack* st, int elem);

bool stack_empty(stack* st);

void stack_clear(stack* st);

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

	printf("last elem is (must be 4) %d\n", stack_peek(&st));

	stack_pop(&st);
	printf("last elem is (must be 3) %d\n", stack_peek(&st));

	stack_pop(&st);
	printf("last elem is (must be 2) %d\n", stack_peek(&st));

	stack_pop(&st);

	printf("check if stack is empty  (must be 0)%d\n", stack_empty(&st));

	stack_pop(&st);

	printf("check if stack is empty  (must be 1)%d\n", stack_empty(&st));



	return 0;
}


*/