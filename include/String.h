#ifndef STRING_H
#define STRING_H

typedef struct {
	size_t length;
	char content[];
} String;

String* String_new(size_t length);

#endif
