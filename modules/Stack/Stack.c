#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "Stack.h"

// function to allocate memory for a new Stack
Stack *Stack_new() {
	Stack *new_stack = malloc(sizeof(Stack));
	new_stack->length = 0;
	return new_stack;
}

// function to add an item to the top of an existing Stack by allocating more memory for it
Stack *Stack_push(Stack *stack, void *new_element) {
	stack = realloc(stack, sizeof(Stack) + (stack->length + 1) * sizeof(void*));
	stack->content[stack->length] = new_element;
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
	bool shifting = false;

	for (size_t i = 0; i < stack->length; i++) {
		// all items above the deleted item will be shifted down
		if (shifting) {
			stack->content[i - 1] = stack->content[i];
		}

		// once the doomed item is reached, begin shifting items down
		if (i == index) {
			shifting = true;
		}
	}

	// change the length of the stack to reflect the updated contents
	stack->length--;
	stack = realloc(stack, sizeof(Stack) + stack->length * sizeof(void*));
}
