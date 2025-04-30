#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "String.h"

// function to allocate memory for a new String
String *String_new(size_t length) {
	String *new_string = malloc(sizeof(String) + length * sizeof(char));
	new_string->length = length;
	return new_string;
}

// function to check if the contents of a string match a constant char array
// useful for determining what operation is represented by a token
bool String_is(String *string, char *char_array) {
	size_t i = 0;

	// iterate through the string and the char array
	for (; i < string->length; i++) {
		// if at any point the char array and the string differ, or if we have reached the end of the char array but not the string, then the two objects do not store the same value
		if (char_array[i] == '\0' || char_array[i] != string->content[i]) {
			return false;
		}
	}

	// if we have not reached the end of the char array but we have reached the end of the string, then the two objects do not store the same value
	if (char_array[i] != '\0') {
		return false;
	}

	// if everything else doesn't fail, then the two objects must store the same value
	return true;
}

// function to check if a string contains a certain character
bool String_has_char(String* string, char character) {
	// go through each character in the string
	for (size_t i = 0; i < string->length; i++) {
		// if the current character matches the character we're searching for, then it's in the string
		if (string->content[i] == character) {
			return true;
		}
	}

	// if we have reached the end of the string and still haven't found it, then the character is not in the string
	return false;
}

// function to append a character to a String by reallocating the memory and assigning the character to the last byte
String *String_append_char(String *string, char character) {
	string = realloc(string, sizeof(String) + (string->length + 1) * sizeof(char));
	string->content[string->length] = character;
	string->length++;
	return string;
}

// function to print a String to the console
void String_print(String *string) {
	for (size_t i = 0; i < string->length; i++) {
		putchar(string->content[i]);
	}
}
