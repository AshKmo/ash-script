#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "String.h"
#include "Element.h"

// function to evaluate a script string and return the result
Element* eval(String *script, Element *scopes) {
}

// function to return a new String object containing the contents of a file
String* read_file(char *path) {
	// open the script file for reading
	FILE *file_pointer = fopen(path, "rb");

	// seek to the end and get the position after the last character
	fseek(file_pointer, 0, SEEK_END);
	long file_size = ftell(file_pointer);

	// return to the start to read the file
	fseek(file_pointer, 0, SEEK_SET);

	String *file_content = String_new(file_size);

	for (size_t i = 0; i < file_size; i++) {
		file_content->content[i] = fgetc(file_pointer);
	}

	// close the file so it doesn't reside in memory forever
	fclose(file_pointer);

	return file_content;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		puts("Please provide a script file as an argument.");
	}

	String *script = read_file(argv[1]);

	eval(script, NULL);

	// free the memory that the script uses because we won't need it again
	free(script);

	return 0;
}
