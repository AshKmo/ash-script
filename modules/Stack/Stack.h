#ifndef STACK_H
#define STACK_H

typedef struct {
	size_t length;
	void *content[];
} Stack;

Stack *Stack_new();

Stack *Stack_push(Stack*, void*);

Stack *Stack_pop(Stack*);

Stack *Stack_delete(Stack*, size_t);

#endif
