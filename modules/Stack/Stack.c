// module that adds a Stack type that can dynamically grow and shrink as items are pushed to and popped from it

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "Stack.h"

// function to make and initialise a new Stack
Stack *Stack_new() {
	Stack *new_stack = malloc(sizeof(Stack));
	new_stack->length = 0;
	return new_stack;
}

// function to add an item to the top of an existing Stack by allocating more memory for it
Stack *Stack_push(Stack *stack, void *new_element) {
	stack = realloc(stack, sizeof(Stack) + (stack->length + 1) * sizeof(void*));
	stack->content[stack->length] = new_element;
	stack->length++;
	return stack;
}

// function to remove the top item from a Stack by deallocating some of the Stack's memory
Stack *Stack_pop(Stack *stack) {
	stack->length--;
	stack = realloc(stack, sizeof(Stack) + stack->length * sizeof(void*));
	return stack;
}

// function to delete an item from a stack at an arbitrary position
Stack *Stack_delete(Stack *stack, size_t index) {
	// shift every item after doomed item back one position
	for (size_t i = index + 1; i < stack->length; i++) {
		stack->content[i - 1] = stack->content[i];
	}

	// change the length of the stack to reflect the updated contents
	stack->length--;
	stack = realloc(stack, sizeof(Stack) + stack->length * sizeof(void*));

	return stack;
}
