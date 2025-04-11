#include <stdbool.h>

#ifndef ELEMENT_H
#define ELEMENT_H

typedef enum {
	ELEMENT_NOTHING,
} ElementType;

typedef struct {
	ElementType type;
	bool has_been_checked;
	bool not_potential_garbage;
	void *value;
} Element;

Element* Element_new(ElementType type, void *value);

#endif
