#include <stdlib.h>

#include "Element.h"

Element* Element_new(ElementType type, void *value) {
	Element *new_element = malloc(sizeof(Element));
	new_element->type = type;
	new_element->value = value;
	return new_element;
}
