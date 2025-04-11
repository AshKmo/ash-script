#include <stddef.h>
#include <stdlib.h>

#include "String.h"

String* String_new(size_t length) {
	String *new_string = malloc(sizeof(String));
	new_string->length = length;
	return new_string;
}
