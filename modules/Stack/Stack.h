// reusable component that adds a Stack type that can dynamically grow and shrink as items are pushed to and popped from it

#ifndef STACK_H
#define STACK_H

typedef struct {
	size_t length;
	void *content[];
} Stack;

Stack *Stack_new();

Stack *Stack_push(Stack*, void*);

#endif
