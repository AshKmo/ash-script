// the ash-script programming language interpreter
// written by Ashley Kollmorgen, 2025
// https://ashkmo.dedyn.io

// import the necessary built-in libraries
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

// import additional modules
#include "String.h"
#include "Stack.h"

// function to spit out an error and kill the program if/when necessary
void whoops(char *reason) {
	fputs("ERROR: ", stderr);
	fputs(reason, stderr);
	fputc('\n', stderr);
	exit(1);
}

// function to return a new String object containing the contents of a file
String *read_file(char *path) {
	// open the script file for reading, or return NULL if it can't be opened
	FILE *file_pointer = fopen(path, "rb");
	if (file_pointer == NULL) {
		return NULL;
	}

	// seek to the end and get the position after the last character
	fseek(file_pointer, 0, SEEK_END);
	long file_size = ftell(file_pointer);

	// return to the start to read the file
	fseek(file_pointer, 0, SEEK_SET);

	// make a new string to store the script
	String *file_content = String_new(file_size);

	// store the file contents in the string
	fread(file_content->content, 1, file_size, file_pointer);

	// close the file so it doesn't reside in memory forever
	fclose(file_pointer);

	return file_content;
}

// function to attempt to write a String to a file and return true only if the operation was successful
bool write_file(char *path, String *new_contents) {
	// try to open the file for writing and return false if this doesn't happen
	FILE *file_pointer = fopen(path, "wb");
	if (file_pointer == NULL) {
		return false;
	}

	// return true only if the entire contents were successfully written to the file
	bool result = fwrite(new_contents->content, 1, new_contents->length, file_pointer) == new_contents->length;

	// close the file so it doesn't reside in memory forever
	fclose(file_pointer);

	return result;
}

// numeric type that can represent either long integer or double-precision floating-point values
typedef struct {
	bool is_double;
	union {
		long value_long;
		double value_double;
	};
} Number;

// function to allocate memory for a new Number
Number *Number_new() {
	Number *number = malloc(sizeof(Number));
	number->is_double = false;
	number->value_long = 0;
	return number;
}

// enumeration type used to represent the type of an Element
typedef enum {
	ELEMENT_NOTHING,

	ELEMENT_TERMINATOR,
	ELEMENT_BRACKET,
	ELEMENT_BRACE,

	ELEMENT_NULL,
	ELEMENT_VARIABLE,
	ELEMENT_OPERATION,
	ELEMENT_STRING,
	ELEMENT_NUMBER,

	ELEMENT_SEQUENCE,

	ELEMENT_SCOPE_COLLECTION,
	ELEMENT_SCOPE,
	ELEMENT_CLOSURE,
} ElementType;

// type used to store any value that can be encountered by the language, as well as its type and garbage-collection status
typedef struct {
	ElementType type;
	bool gc_checked;
	void *value;
} Element;

// function to allocate memory for a new Element
Element *Element_new(ElementType type, void *value) {
	Element *new_element = malloc(sizeof(Element));
	new_element->type = type;
	new_element->gc_checked = false;
	new_element->value = value;
	return new_element;
}

// enumeration type used to represent the type of an Operation
typedef enum {
	OPERATION_APPLICATION,
	OPERATION_ACCESS,
	OPERATION_POW,
	OPERATION_MULTIPLICATION,
	OPERATION_DIVISION,
	OPERATION_REMAINDER,
	OPERATION_ADDITION,
	OPERATION_SUBTRACTION,
	OPERATION_SHIFT_LEFT,
	OPERATION_SHIFT_RIGHT,
	OPERATION_SUBL,
	OPERATION_SUBG,
	OPERATION_LT,
	OPERATION_GT,
	OPERATION_LTE,
	OPERATION_GTE,
	OPERATION_EQUALITY,
	OPERATION_INEQUALITY,
	OPERATION_AND,
	OPERATION_XOR,
	OPERATION_OR,
	OPERATION_CLOSURE,
} OperationType;

// array storing the precedence value of each operator
const int OPERATOR_PRECEDENCE[] = {0, 0, 1, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8, 9, 9, 10, 11, 12, 13};

// type used to represent an operation that is to be performed on two values
typedef struct {
	OperationType type;
	Element *a;
	Element *b;
} Operation;

// function to allocate memory for a new Operation
Operation *Operation_new(OperationType type, Element *a, Element *b) {
	Operation *new_operation = malloc(sizeof(Operation));
	new_operation->type = type;
	new_operation->a = a;
	new_operation->b = b;
	return new_operation;
}

// function to perform operations on numbers
Number* Number_operate(OperationType operation_type, Number *number_a, Number *number_b) {
	Number *result = Number_new();

	// the result should usually be a floating-point value if either operand is one already
	result->is_double = number_a->is_double || number_b->is_double;


	// most of these operations follow the same format:
	// if one of the operands is floating-point, then perform the operation using the appropriate property for each number (value_double or value_long) depending on the type of number it is
	// otherwise, just perform the operation using value_long for both numbers
	switch (operation_type) {
		case OPERATION_ADDITION:
			if (result->is_double) {
				result->value_double =
					(number_a->is_double ? number_a->value_double : number_a->value_long) +
					(number_b->is_double ? number_b->value_double : number_b->value_long);
			} else {
				result->value_long = number_a->value_long + number_b->value_long;
			}
			break;
		case OPERATION_SUBTRACTION:
			if (result->is_double) {
				result->value_double =
					(number_a->is_double ? number_a->value_double : number_a->value_long) -
					(number_b->is_double ? number_b->value_double : number_b->value_long);
			} else {
				result->value_long = number_a->value_long - number_b->value_long;
			}
			break;
		case OPERATION_MULTIPLICATION:
			if (result->is_double) {
				result->value_double =
					(number_a->is_double ? number_a->value_double : number_a->value_long) *
					(number_b->is_double ? number_b->value_double : number_b->value_long);
			} else {
				result->value_long = number_a->value_long * number_b->value_long;
			}
			break;
		case OPERATION_DIVISION:
			// if the denominumber_ator is zero or there is no clean integer division, make the result floating-point
			if (number_b->value_long == 0 || number_a->value_long % number_b->value_long != 0) {
				result->is_double = true;
			}

			if (result->is_double) {
				// if the result needs to be floating-point, convert the operands to floating-point
				double da = number_a->is_double ? number_a->value_double : number_a->value_long;
				double db = number_b->is_double ? number_b->value_double : number_b->value_long;

				// if the denominumber_ator is zero, use the appropriate infinity value
				// otherwise, perform regular floating-point division
				result->value_double = db == 0 ? (da == 0 ? NAN : da > 0 ? INFINITY : -INFINITY) : da / db;
			} else {
				// if the two values are normal integers and the denominumber_ator isn't zero, just use integer division
				result->value_long = number_a->value_long / number_b->value_long;
			}
			break;

		case OPERATION_REMAINDER:
			// the remainder operator only applies to integers
			if (result->is_double) {
				whoops("cannot apply remainder operation to floating-point values");
			} else {
				result->value_long = number_a->value_long % number_b->value_long;
			}
			break;

		case OPERATION_POW:
			// the pow() function only works on doubles and only returns a double, so the result is a double
			result->is_double = true;

			// convert each operand to a double
			double da = number_a->is_double ? number_a->value_double : number_a->value_long;
			double db = number_b->is_double ? number_b->value_double : number_b->value_long;

			result->value_double = pow(da, db);
			break;

		case OPERATION_LT:
		case OPERATION_GT:
			// switch the operands around for the other operator so we don't have to write the same logic twice
			if (operation_type == OPERATION_GT) {
				Number *temp = number_a;
				number_a = number_b;
				number_b = temp;
			}

			// the result will only be 1/0 for true/false so it should be an integer
			result->is_double = false;

			result->value_long =
				(number_a->is_double ? number_a->value_double : number_a->value_long) <
				(number_b->is_double ? number_b->value_double : number_b->value_long);
			break;

		case OPERATION_LTE:
		case OPERATION_GTE:
			// switch the operands around for the other operator so we don't have to write the same logic twice
			if (operation_type == OPERATION_GTE) {
				Number *temp = number_a;
				number_a = number_b;
				number_b = temp;
			}

			// the result will only be 1/0 for true/false so it should be an integer
			result->is_double = false;

			result->value_long =
				(number_a->is_double ? number_a->value_double : number_a->value_long) <=
				(number_b->is_double ? number_b->value_double : number_b->value_long);
			break;
	}

	return result;
}

// type used to represent an association between a key and a value
typedef struct {
	Element *key;
	Element *value;
} Map;

// type used to represent a scope of variables or an object with properties depending on usage
typedef struct {
	size_t length;
	Map maps[];
} Scope;

// function to allocate memory for a new Scope object
Scope *Scope_new() {
	Scope *new_scope = malloc(sizeof(Scope));
	new_scope->length = 0;
	return new_scope;
}

// forward declaration of Scope_get() for mutual recursion
Element *Scope_get(Scope*, Element*);

// function to compare two Elements
bool Element_compare(Element *a, Element *b) {
	// handle the case where at least one of the elements is invalid
	if (a == NULL || b == NULL) {
		return false;
	}

	// if the same Element is being passed to both arguments, then they are equal
	if (a == b) {
		return true;
	}

	// elements must be of the same type to be equal
	if (a->type != b->type) {
		return false;
	}

	switch (a->type) {
		case ELEMENT_NULL:
			// there is only one value that null Elements can be, and we already know that the type is the same, so these elements must be equal
			return true;

		case ELEMENT_VARIABLE:
		case ELEMENT_STRING:
			// string-like values can be compared by comparing their lengths and each character in them
			{
				String *string_a = a->value;
				String *string_b = b->value;

				// if the lengths are different, the strings are different
				if (string_a->length != string_b->length) {
					return false;
				}

				// iterate through each character in both strings and if any character differs, then the strings differ
				for (size_t i = 0; i < string_a->length; i++) {
					if (string_a->content[i] != string_b->content[i]) {
						return false;
					}
				}

				// if each corresponding character is equal and the lengths are equal, the string must be equal
				return true;
			};
			break;

		case ELEMENT_NUMBER:
			{
				Number *number_a = a->value;
				Number *number_b = b->value;

				// each number can be either a double or long, so each combination of doubles and longs must be considered
				if (number_a->is_double) {
					if (number_b->is_double) {
						return number_a->value_double == number_b->value_double;
					} else {
						return number_a->value_double == number_b->value_long;
					}
				} else {
					if (number_b->is_double) {
						return number_a->value_long == number_b->value_double;
					} else {
						return number_a->value_long == number_b->value_long;
					}
				}
			};
			break;

		case ELEMENT_SCOPE:
			{
				Scope *scope_a = a->value;
				Scope *scope_b = b->value;

				// scopes with differing numbers of mappings must not be equal
				if (scope_a->length != scope_b->length) {
					return false;
				}

				// the scopes are not equal if any of the mappings differ
				for (size_t i = 0; i < scope_a->length; i++) {
					if (!Element_compare(Scope_get(scope_b, scope_a->maps[i].key), scope_a->maps[i].value)) {
						return false;
					}
				}

				// if the lengths are the same and the mappings are the same, the scopes must be equal
				return true;
			};
			break;
	}

	// Elements that for whatever reason cannot be compared are considered non-equal
	return false;
}

// function to edit the mapping within a Scope for a certain key, creating one if it doesn't exist yet
Scope *Scope_set(Scope *scope, Element *key, Element *value) {
	// iterate through the scope and update the value of a matching Map if one is found
	for (size_t i = 0; i < scope->length; i++) {
		if (Element_compare(scope->maps[i].key, key)) {
			scope->maps[i].value = value;
			return scope;
		}
	}

	// if no matching Map is found, more memory must be allocated for an additional Map
	scope = realloc(scope, sizeof(Scope) + (scope->length + 1) * sizeof(Map));

	// configure the properties of the new Map so that it maps the key to the new value
	scope->maps[scope->length].key = key;
	scope->maps[scope->length].value = value;

	scope->length++;

	return scope;
}

// function to get a value mapped to a certain key in a Scope
Element *Scope_get(Scope *scope, Element *key) {
	// search through all mappings in the Scope and return the value of one if the keys match
	for (size_t i = 0; i < scope->length; i++) {
		if (Element_compare(scope->maps[i].key, key)) {
			return scope->maps[i].value;
		}
	}

	// otherwise, there is no matching element, so return NULL
	return NULL;
}

// function to determine whether or not a mapping is present within a Scope with a matching key
bool Scope_has(Scope *scope, Element *key) {
	// search through all mappings in the Scope and return true if one with a matching key is present
	for (size_t i = 0; i < scope->length; i++) {
		if (Element_compare(scope->maps[i].key, key)) {
			return true;
		}
	}

	// no matches were found
	return false;
}

// function to delete a mapping in a Scope
bool Scope_delete(Scope *scope, Element *key) {
	bool shift_back = false;

	// iterate through the Scope until the matching mapping is found
	// after that, shift every mapping back one place
	for (size_t i = 0; i < scope->length; i++) {
		if (shift_back) {
			scope->maps[i - 1].key = scope->maps[i].key;
			scope->maps[i - 1].value = scope->maps[i].value;
		} else if (Element_compare(scope->maps[i].key, key)) {
			shift_back = true;
		}
	}

	// if no matching key was ever found, do nothing
	if (!shift_back) {
		return false;
	}

	// reduce the size of the scope by one mapping's worth
	scope->length--;
	scope = realloc(scope, sizeof(Scope) + scope->length * sizeof(Map));

	return scope;
}

// function to print Elements to the console based on their type
void Element_print(Element *element, int indentation, bool literal) {
	// make sure that the Element actually exists first
	if (element == NULL) {
		return;
	}

	switch (element->type) {
		case ELEMENT_NULL:
			putchar('?');
			break;

		case ELEMENT_NUMBER:
			{
				Number *number = element->value;

				if (number->is_double) {
					// if the number is a floating-point value, print it to the maximum number of decimal places necessary to represent it exactly
					printf("%.*g", DBL_DECIMAL_DIG, number->value_double);
				} else {
					// if the number is a long integer, just print it
					printf("%d", number->value_long);
				}
			};
			break;

		case ELEMENT_VARIABLE:
			String_print(element->value);
			break;

		case ELEMENT_STRING:
			if (literal) {
				// if we are supposed to print the Element as a literal element, print it as a correctly-formatted ash-script string

				String *string = element->value;

				putchar('"');

				for (size_t i = 0; i < string->length; i++) {
					// when a quote is found, it should be escaped by a backslash
					if (string->content[i] == '"') {
						putchar('\\');
					}

					putchar(string->content[i]);
				}

				putchar('"');
			} else {
				// otherwise, just dump the contents to the console
				String_print(element->value);
			}
			break;

		case ELEMENT_SCOPE:
			{
				Scope *scope = element->value;

				putchar('{');
				putchar('\n');

				// iterate through all the mappings in the Scope
				for (size_t i = 0; i < scope->length; i++) {
					// correctly indent each line of the Scope
					for (int i = 0; i < indentation + 1; i++) {
						putchar('\t');
					}

					// print a 'let' statement defining each key and value in the scope
					fputs("let ", stdout);
					Element_print(scope->maps[i].key, indentation + 1, false);
					putchar(' ');
					Element_print(scope->maps[i].value, indentation + 1, false);
					putchar(';');
					putchar('\n');
				}

				// correctly indent the last line of the Scope
				for (int i = 0; i < indentation; i++) {
					putchar('\t');
				}

				putchar('}');
			};
			break;

		case ELEMENT_CLOSURE:
			// Closures can't easily be represented due to their Scope-containing nature, so we're just going to have to print a placeholder
			fputs("[=>]", stdout);
			break;

		default:
			// if the Element does not have its type listed above, print a placeholder indicating its type
			printf("[WEIRD %d]", element->type);
	}
}

// function to determine whether or not an Element has a truthy value
bool Element_is_truthy(Element *element) {
	switch (element->type) {
		case ELEMENT_NULL:
			// Null Elements are never truthy
			return false;

		case ELEMENT_NUMBER:
			// Number Elements are truthy if they do not equal 0 or NAN
			{
				Number *number = element->value;

				if (number->is_double) {
					return number->value_double != 0 && number->value_double != NAN;
				} else {
					return number->value_long != 0;
				}
			};
			break;
		case ELEMENT_STRING:
			// String Elements are truthy if they are not empty
			{
				String *string = element->value;

				return string->length != 0;
			};
			break;
		case ELEMENT_SCOPE:
			// Scope Elements are truthy if they contain at least one mapping
			{
				Scope *scope = element->value;

				return scope->length != 0;
			};
			break;
	}

	// all Elements of any other type are considered truthy
	return true;
}

// type used to represent a Closure, which is a function that stores within itself the Scopes surrounding it
// this means that variables in the Scopes in which the Closure was created can be accessed by the Closure when it is called
typedef struct {
	Element *expression;
	Element *variable;
	Element *scopes;
} Closure;

// function to allocate memory for a new Closure object
Closure *Closure_new(Element *expression, Element *variable, Element *scopes) {
	Closure *new_closure = malloc(sizeof(Closure));
	new_closure->expression = expression;
	new_closure->variable = variable;
	new_closure->scopes = scopes;
	return new_closure;
}

// function to convert a hex digit into the number it represents (returned as a char)
char hex_char(char c) {
	// uppercase letters represent 10-15, so get the index of the letter relative to 'A' and add 10
	if (c >= 97) {
		return c - 97 + 10;
	}

	// lowercase letters also represent 10-15, so get the index of the letter relative to 'a' and add 10
	if (c >= 65) {
		return c - 65 + 10;
	}

	// decimal digits represent 0-9, so just get the index of the digit relative to '0'
	return c - 48;
}

// function that makes a new element with a specific type and value, and pushes it to a stack so that it can be garbage collected later
Element *make(ElementType type, void *value, Stack **heap) {
	Element *new_element = Element_new(type, value);
	*heap = Stack_push(*heap, new_element);
	return new_element;
}

// function to convert a script string into a list of tokens that an abstract syntax tree can be easily constructed from
Stack *tokenise(String *script, Stack **heap) {
	// stack to store the new tokens
	Stack *tokens = Stack_new();

	// these store the type and value of the current element we are dealing with
	ElementType current_type;
	String *current_value = String_new(0);

	// whether we are in an escape sequence or not
	bool escaped = false;
	// whether we are in a comment or not
	bool comment = false;
	// whether we are inside a string or not
	bool in_string = false;

	// iterate through each character in the script, plus an extra non-existent newline to keep the tokenising logic simple
	for (size_t i = 0; i <= script->length; i++) {
		// stores the implied element type of the element at the current character
		ElementType new_type;

		// stores the current character we are dealing with
		char c;

		// if we're at the non-existent extra newline, deal with it
		if (i == script->length) {
			c = '\n';
		} else {
			c = script->content[i];
		}

		// if there's a backslash and we're not currently in an escape sequence it's probably the start of an escape sequence
		if (c == '\\' && !escaped) {
			escaped = true;
			continue;
		}

		// if there's a hash mark and we're not currently in an escape sequence it's probably the start or end of a comment
		if (c == '#' && !escaped) {
			comment = !comment;
			continue;
		}

		// if we're in a comment then there's not much else to do
		if (comment) {
			continue;
		}

		// escaped and string characters need special handling
		if (escaped || in_string) {
			// if we're not in a string but the current character is escaped, treat it as part of a variable name
			new_type = ELEMENT_VARIABLE;

			// if we're in a string, treat the character literally unless it's part of a special escape sequence
			if (in_string) {
				new_type = ELEMENT_STRING;

				if (escaped) {
					switch (c) {
						case 'n':
							// '\n' evaluates to a newline
							c = '\n';
							break;
						case 'r':
							// '\r' evaluates to a carriage return
							c = '\r';
							break;
						case 't':
							// '\t' evaluates to a tab
							c = '\t';
							break;
						case 'x':
							// '\xHH' evaluates to the character with hex value HH

							// the first digit represents the 16s place and the second represents the 1s place
							c = 16 * hex_char(script->content[i + 1]) + hex_char(script->content[i + 2]);
							// we've scanned ahead here, so update the index accordingly
							i += 2;
							break;
					}
				} else if (c == '"') {
					new_type = ELEMENT_NOTHING;
				}
			}
		} else {
			// since we're not in a string or escape sequence, treat each character according to what it represents
			switch (c) {
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					// whitespace terminates the current token so it has a special element type
					new_type = ELEMENT_NOTHING;
					break;

				case ';':
					// semicolons are statement terminators
					new_type = ELEMENT_TERMINATOR;
					break;

				case '(':
				case ')':
					// braces group sequences
					new_type = ELEMENT_BRACKET;
					break;

				case '{':
				case '}':
					// brackets group expressions
					new_type = ELEMENT_BRACE;
					break;

				case '"':
					// quotes initiate or terminate a string
					new_type = ELEMENT_STRING;
					break;

				case '+':
				case '*':
				case '/':
				case '%':
				case '=':
				case '<':
				case '>':
				case '&':
				case '|':
				case '^':
				case '!':
				case '$':
					// these special characters are all part of operators
					new_type = ELEMENT_OPERATION;
					break;

				case '.':
					// full stops can be part of a number but otherwise they are operators
					if (current_type != ELEMENT_NUMBER) {
						new_type = ELEMENT_OPERATION;
					}
					break;

				case '-':
					// dashes can indicate the start of a negative number but otherwise they are operators
					// this is because I don't want to implement unary operations as well

					if (i < script->length - 1 && isdigit(script->content[i + 1])) {
						new_type = ELEMENT_NUMBER;
					} else {
						new_type = ELEMENT_OPERATION;
					}
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					// decimal digits can be part of variable names but otherwise they represent numbers
					if (current_type != ELEMENT_VARIABLE) {
						new_type = ELEMENT_NUMBER;
					}
					break;

				case '?':
					// question marks represent the null value
					new_type = ELEMENT_NULL;
					break;

				default:
					// all other characters can be part of valid variable names
					new_type = ELEMENT_VARIABLE;
					break;
			}
		}

		// if we've reached the end of a token, we need to add it to the token list and start making a new token
		// we've reached the end of a token if we've got one to begin with and if there's been a change in the token type
		// brackets, braces and semicolons can also trigger the end of a token, since they cannot be part of a variable name
		if ((current_value->length > 0 || in_string) && (new_type != current_type ||
					current_type == ELEMENT_BRACKET ||
					current_type == ELEMENT_BRACE ||
					current_type == ELEMENT_TERMINATOR
					)) {

			// add a new element to the program's internally-managed heap
			Element *new_token = make(current_type, NULL, heap);

			// figure out the value of the token based on its type
			switch (current_type) {
				case ELEMENT_NULL:
				case ELEMENT_TERMINATOR:
					// nulls and terminators don't contain any meaningful characters
					free(current_value);
					break;

				case ELEMENT_VARIABLE:
				case ELEMENT_STRING:
					// variables and strings contain their own characters
					new_token->value = current_value;
					break;

				case ELEMENT_BRACKET:
				case ELEMENT_BRACE:
					// brackets and braces only need to keep track of whether they are opening or closing
					{
						char bracket_character = current_value->content[0];

						// usually, Element structs use their 'value' pointer to point to the value of the element
						// however, since bracket and brace elements are only either opening or closing, I figured it would be slightly more efficient to store this within the pointer value itself, instead of making another object on the heap
						new_token->value = (void*)(uintptr_t)(bracket_character == '}' || bracket_character == ')');

						// brackets and braces don't need to store their characters
						free(current_value);
					};
					break;

				case ELEMENT_OPERATION:
					// operators need to become operation elements
					{
						// make the new empty operation element
						Operation *operation = Operation_new(OPERATION_APPLICATION, NULL, NULL);

						// match the new operator to its operation
						// I apologise for this ugly monstrosity but it's necessary because C can't switch-case with strings
						if (String_is(current_value, "+")) {
							operation->type = OPERATION_ADDITION;
						} else if (String_is(current_value, "-")) {
							operation->type = OPERATION_SUBTRACTION;
						} else if (String_is(current_value, "*")) {
							operation->type = OPERATION_MULTIPLICATION;
						} else if (String_is(current_value, "/")) {
							operation->type = OPERATION_DIVISION;
						} else if (String_is(current_value, "%")) {
							operation->type = OPERATION_REMAINDER;
						} else if (String_is(current_value, "==")) {
							operation->type = OPERATION_EQUALITY;
						} else if (String_is(current_value, "<")) {
							operation->type = OPERATION_LT;
						} else if (String_is(current_value, ">")) {
							operation->type = OPERATION_GT;
						} else if (String_is(current_value, "<=")) {
							operation->type = OPERATION_LTE;
						} else if (String_is(current_value, ">=")) {
							operation->type = OPERATION_GTE;
						} else if (String_is(current_value, "!=")) {
							operation->type = OPERATION_INEQUALITY;
						} else if (String_is(current_value, "<<")) {
							operation->type = OPERATION_SHIFT_LEFT;
						} else if (String_is(current_value, ">>")) {
							operation->type = OPERATION_SHIFT_RIGHT;
						} else if (String_is(current_value, "&")) {
							operation->type = OPERATION_AND;
						} else if (String_is(current_value, "|")) {
							operation->type = OPERATION_OR;
						} else if (String_is(current_value, "^")) {
							operation->type = OPERATION_XOR;
						} else if (String_is(current_value, "**")) {
							operation->type = OPERATION_POW;
						} else if (String_is(current_value, ">/")) {
							operation->type = OPERATION_SUBG;
						} else if (String_is(current_value, "</")) {
							operation->type = OPERATION_SUBL;
						} else if (String_is(current_value, ".")) {
							operation->type = OPERATION_ACCESS;
						} else if (String_is(current_value, "=>")) {
							operation->type = OPERATION_CLOSURE;
						}

						// set the value of the token to its new Operation object
						new_token->value = operation;

						// we're done with the value string now
						free(current_value);
					};
					break;

				case ELEMENT_NUMBER:
					// numbers need to be converted to Number elements that store integers or floats
					{
						Number *number = Number_new();

						// copy the number contents into a null-terminated char array so we can process them using atof and atoi
						char number_string[current_value->length + 1];
						memcpy(number_string, current_value->content, current_value->length);
						number_string[current_value->length] = '\0';

						// if there's a decimal point then it's floating-point, otherwise it's an integer
						if (String_has_char(current_value, '.')) {
							number->is_double = true;
							number->value_double = atof(number_string);
						} else {
							number->value_long = atoi(number_string);
						}

						// set the value of the token to its new Number object
						new_token->value = number;

						// we're done with the value string now
						free(current_value);
					};
					break;
			}

			// push the new token onto the stack
			tokens = Stack_push(tokens, new_token);

			// reset the token variables so we can make a new one
			current_type = ELEMENT_NOTHING;
			current_value = String_new(0);
		}

		// we no longer need to check for differences in token types, so update the current type
		current_type = new_type;

		// if the current character is not whitespace and we are either fully inside or fully outside a string, add the character to the current token
		if (current_type != ELEMENT_NOTHING && (in_string || current_type != ELEMENT_STRING)) {
			current_value = String_append_char(current_value, c);
		}

		// if we've just entered a String, set the in_string boolean to reflect that
		in_string = current_type == ELEMENT_STRING;

		// treat the next character as having not been escaped, since we already dealt with the escape sequence
		escaped = false;
	}

	// free up the memory used for temporarily storing token values
	free(current_value);

	return tokens;
}

// function to handle the construction of a hierarchy of operations from a list of tokens
Element *operatify(Stack *expression, size_t start, size_t end, Stack **heap) {
	// if there is only one element in the expression, then it's the return value
	if (end - start == 1) {
		return expression->content[start];
	}

	Element *final_element = NULL;

	Operation *final_operation = NULL;

	size_t operation_location = 0;

	// iterate through all the possible operators and find the one with the worst (highest) precedence value
	int precedence_record = 1;
	for (size_t i = start; i < end; i++) {
		Element *current_token = expression->content[i];

		if (current_token->type == ELEMENT_OPERATION) {
			Operation *current_operation = current_token->value;

			// the operator with the worst precedence and that occurs last should be selected first
			bool satisfactory_precedence = OPERATOR_PRECEDENCE[current_operation->type] >= precedence_record;

			// certain operations are right-associative and so the last of them in the chain should be executed last
			if (final_operation != NULL && final_operation->type == OPERATION_CLOSURE) {
				satisfactory_precedence = OPERATOR_PRECEDENCE[current_operation->type] > precedence_record;
			}

			// only select operators that we haven't handled yet and that have an appropriately bad level of precedence
			if (current_operation->a == NULL && satisfactory_precedence) {
				// update the variables to point to the new record-holding operator
				final_element = current_token;
				final_operation = current_operation;
				precedence_record = OPERATOR_PRECEDENCE[current_operation->type];
				operation_location = i;
			}
		}
	}

	// if no operation was found, then this expression must entirely consist of property access operators and/or application by juxtaposition
	if (final_operation == NULL) {
		Element *second_last_element = expression->content[end - 2];

		if (second_last_element->type == ELEMENT_OPERATION) {
			// if the second last element is an operation, then it must be an access operation
			final_element = second_last_element;
			final_operation = second_last_element->value;
			operation_location = end - 2;
		} else {
			// otherwise, treat it as application by juxtaposition
			return make(ELEMENT_OPERATION, Operation_new(OPERATION_APPLICATION, operatify(expression, start, end - 1, heap), expression->content[end - 1]), heap);
		}
	}

	// if an operation was found, then process everything to the left of it and use it as the operation's first value
	final_operation->a = operatify(expression, start, operation_location, heap);

	// if an operation was found, then process everything to the right of it and use it as the operation's second value
	final_operation->b = operatify(expression, operation_location + 1, end, heap);

	return final_element;
}

// forward declaration of elementify_sequence() for mutual recursion
Element *construct_sequence(Stack*, size_t*, Stack**);

// function to handle the construction of the abstract syntax tree branches of expressions
Element *construct_expression(Stack *tokens, size_t *i, Stack **heap) {
	Stack *expression = Stack_new();

	bool end_of_expression = false;

	// iterate through each token in the list
	for (; *i < tokens->length && !end_of_expression; (*i)++) {
		Element *current_token = tokens->content[*i];

		if (current_token->type == ELEMENT_BRACKET && (uintptr_t)(current_token->value)) {
			// the loop will increment the position integer once after it exits so we're going to have to nudge it down so that the tokens after the bracket are not missed
			(*i)--;

			end_of_expression = true;
		} else {
			switch (current_token->type) {
				case ELEMENT_BRACE:
					// a brace signifies the start of a sequence

					// move to the next character and encapsulate the tokens following the brace in a single element
					(*i)++;
					expression = Stack_push(expression, construct_sequence(tokens, i, heap));
					break;

				case ELEMENT_BRACKET:
					// since we already checked for the closing brace, this one must be an opening brace, signifying the start of a new sequence

					// move to the next character and encapsulate the tokens following the bracket in a single element
					(*i)++;
					expression = Stack_push(expression, construct_expression(tokens, i, heap));
					break;

				default:
					// everything else should just be inserted into the expression as an operator or value
					expression = Stack_push(expression, current_token);
					break;
			}
		}
	}

	// convert the new list of tokens to a proper expression by constructing the hierarchy of operations
	Element *result = operatify(expression, 0, expression->length, heap);

	// free the now-redundant list
	free(expression);

	return result;
}

// function to handle the construction of the abstract syntax tree branchs of sequences
Element *construct_sequence(Stack *tokens, size_t *i, Stack **heap) {
	// create a stack to store the sequence of statements
	Stack *sequence = Stack_new();

	// create a stack to store the tokens in each statement
	Stack *statement = Stack_new();

	bool end_of_sequence = false;

	// iterate through each token in the list
	for (; *i < tokens->length && !end_of_sequence; (*i)++) {
		Element *current_token = tokens->content[*i];

		// if we've come across a closing brace then it's the end of the sequence
		if (current_token->type == ELEMENT_BRACE && (uintptr_t)(current_token->value)) {
			// the loop will increment the position integer once after it exits so we're going to have to nudge it down so that the tokens after the bracket are not missed
			(*i)--;

			end_of_sequence = true;
		} else {
			switch (current_token->type) {
				case ELEMENT_BRACE:
					// since we already checked for the closing brace, this one must be an opening brace, signifying the start of a new sequence

					// move to the next character and encapsulate the tokens following the brace in a single element
					(*i)++;
					statement = Stack_push(statement, construct_sequence(tokens, i, heap));
					break;

				case ELEMENT_BRACKET:
					// a bracket signifies the start of an expression

					// move to the next character and encapsulate the tokens following the bracket in a single element
					(*i)++;
					statement = Stack_push(statement, construct_expression(tokens, i, heap));
					break;

				case ELEMENT_TERMINATOR:
					// statement-terminating semicolons should finalise the statement

					// only finalise the statement if there is stuff in it
					if (statement->length >= 0) {
						// add the new statement to the sequence
						sequence = Stack_push(sequence, statement);

						// make a fresh new statement for more expressions
						statement = Stack_new();
					}
					break;

				default:
					// everything else should just be inserted into the statement as a command or argument
					statement = Stack_push(statement, tokens->content[*i]);
					break;
			}
		}
	}

	// the remaining fresh statement can be safely freed, since it has no useful contents
	free(statement);

	// return the completed sequence as a new element
	return make(ELEMENT_SEQUENCE, sequence, heap);
}

// function to construct the abstract syntax tree from the token list
Element *construct_tree(Stack *tokens, Stack **heap) {
	size_t i = 0;
	return construct_sequence(tokens, &i, heap);
}

// function to recursively mark items as non-garbage so that the garbage collector doesn't destroy useful data
void garbage_check(Element *element) {
	// stop the recursion if a valid element isn't provided or if the element has been visited already
	if (element == NULL || element->gc_checked) {
		return;
	}

	// mark the element as non-garbabge
	element->gc_checked = true;

	switch (element->type) {
		case ELEMENT_OPERATION:
			{
				Operation *operation = element->value;
				garbage_check(operation->a);
				garbage_check(operation->b);
			};
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = element->value;

				// iterate through all statements in the sequence
				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					// iterate through all Elements in the statement
					for (size_t x = 0; x < statement->length; x++) {
						garbage_check(statement->content[x]);
					}
				}
			};
			break;
		case ELEMENT_SCOPE_COLLECTION:
			{
				Stack *scope_collection = element->value;

				// iterate through all the scopes in the collection and check them
				for (size_t i = 0; i < scope_collection->length; i++) {
					garbage_check(scope_collection->content[i]);
				}
			};
			break;
		case ELEMENT_SCOPE:
			{
				Scope *scope = element->value;

				// iterate through all the Maps in the Scope and check the keys and values
				for (size_t i = 0; i < scope->length; i++) {
					garbage_check(scope->maps[i].key);
					garbage_check(scope->maps[i].value);
				}
			};
			break;
		case ELEMENT_CLOSURE:
			{
				Closure *closure = element->value;
				garbage_check(closure->expression);
				garbage_check(closure->variable);
				garbage_check(closure->scopes);
			};
			break;
	}
}

// function to free an element and its contents
void Element_nuke(Element *element) {
	switch (element->type) {
		case ELEMENT_NULL:
		case ELEMENT_BRACKET:
		case ELEMENT_BRACE:
			break;

		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = element->value;

				// free all the statements in the sequence
				for (size_t y = 0; y < sequence->length; y++) {
					free(sequence->content[y]);
				}

				// free the sequence itself
				free(sequence);
			};
			break;

		default:
			// most elements only need their value free'd
			free(element->value);
	}

	// free the element itself
	free(element);
}

// function to clean out any unreferenced garbage that has been building up on the heap tracker
void garbage_collect(Element *result, Element *ast_root, Stack **call_stack, Stack **scopes_stack, Stack **heap) {
	if (result != NULL) {
		// mark any single result value as non-garbage
		garbage_check(result);
	}

	if (ast_root != NULL) {
		// mark the abstract syntax tree as non-garbage
		garbage_check(ast_root);
	}

	if (scopes_stack != NULL) {
		// mark all the items in scopes as non-garbage
		for (size_t i = 0; i < (*scopes_stack)->length; i++) {
			garbage_check((*scopes_stack)->content[i]);
		}
	}

	if (call_stack != NULL) {
		// mark all the items in the call stack as non-garbage
		for (size_t i = 0; i < (*call_stack)->length; i++) {
			garbage_check((*call_stack)->content[i]);
		}
	}

	// iterate through all the Elements on the heap tracker
	for (size_t i = 0; i < (*heap)->length; i++) {
		Element *element = (*heap)->content[i];

		if (element->gc_checked) {
			// re-mark non-garbage as potential garbage for next time
			element->gc_checked = false;
		} else {
			// if any Element is found that has not been marked as non-garbage, remove it from the stack and free it
			*heap = Stack_delete(*heap, i);
			Element_nuke(element);

			// since we have just shifted all the eleents after this one in the heap tracker down by one position, shift the current index down by one position
			i--;
		}
	}
}

// function to set a variable in any of the scopes available in the current evaluation
// the local_only parameter forces the variable to be set only in the local scope
void set_variable(Element *key, Element *value, Element *scopes, bool local_only) {
	Stack *scope_collection = scopes->value;

	Element *scope;

	if (local_only) {
		// if the local_only parameter is set, consider only the most recent scope
		scope = scope_collection->content[scope_collection->length - 1];
	} else {
		bool found = false;

		// search through all scopes from oldest to youngest until a scope is found
		for (size_t i = 0; i < scope_collection->length && !found; i++) {
			scope = scope_collection->content[i];

			// if this scope has the key, select it
			if (Scope_has(scope->value, key)) {
				found = true;
			}
		}
	}

	// modify the appropriate scope according to the key and value specified
	scope->value = Scope_set(scope->value, key, value);
}

// function to retrieve the value of a variable in any of the scopes available in the current evaluation
Element *get_variable(Element *key, Element *scopes) {
	Stack *scope_collection = scopes->value;

	// iterate through all scopes from the youngest to the oldest
	// since size_t is an unsigned integer type, it will overflow to a large positive value when it goes below zero, so the condition for this loop should check for a value greater than the Scope collection length
	for (size_t i = scope_collection->length - 1; i < scope_collection->length; i--) {
		Element *scope_element = scope_collection->content[i];

		// retrieve a potential value from the Scope
		Element *result = Scope_get(scope_element->value, key);

		// if a value for the key was found in this scope, return it
		if (result != NULL) {
			return result;
		}
	}

	// if no value was found, print the variable name and an error message
	putchar('\n');
	Element_print(key, 0, true);
	putchar('\n');
	whoops("variable not found");
}

// forward declaration of evaluate() for mutual recursion
Element *evaluate(Element*, Element*, Stack**, Stack**, Stack**);

// function to apply one element to another by juxtaposition
Element *apply(Element *a, Element *b, Element *ast_root, Stack **call_stack, Stack **scopes_stack, Stack **heap) {
	switch (a->type) {
		case ELEMENT_SCOPE:
			// application of a Scope to any value finds the value associated with the key described by the value to which the Scope is applied
			{
				Element *result = Scope_get(a->value, b);
				if (result == NULL) {
					// if no result is found, print the key and an error message
					putchar('\n');
					Element_print(b, 0, true);
					putchar('\n');
					whoops("no such key in this scope");
				}
				return result;
			};
			break;

		case ELEMENT_CLOSURE:
			// application of a Closure to any value calls the Closure with the value
			{
				Closure *closure = a->value;

				Stack *old_scopes = closure->scopes->value;

				// make a copy of the old Scope collection so that future calls of this closure aren't executed with a mutated Scope collection
				Element *scopes_copy = make(ELEMENT_SCOPE_COLLECTION, Stack_new(), heap);
				for (size_t i = 0; i < old_scopes->length; i++) {
					scopes_copy->value = Stack_push(scopes_copy->value, old_scopes->content[i]);
				}

				// if a variable name has been set, make a new scope containing the variable and its value
				if (closure->variable != NULL) {
					Element *scope = make(ELEMENT_SCOPE, Scope_new(), heap);

					scope->value = Scope_set(scope->value, closure->variable, b);

					// add the new scope to the new scope collection
					scopes_copy->value = Stack_push(scopes_copy->value, scope);
				}

				// add the new set of scopes to the Scope collection stack
				*scopes_stack = Stack_push(*scopes_stack, scopes_copy);

				// add the closure to the call stack so that it isn't garbage collected mid-call
				*call_stack = Stack_push(*call_stack, a);

				// evaluate the Closure expression with the new Scope collection
				Element *result = evaluate(closure->expression, ast_root, call_stack, scopes_stack, heap);

				// remove the new Scope collection from the Scope collection stack so the previous Scope collection is restored
				*scopes_stack = Stack_pop(*scopes_stack);

				// the call has ended so it's safe to remove the Closure from the call stack
				*call_stack = Stack_pop(*call_stack);

				return result;
			};
			break;

		case ELEMENT_STRING:
			// application of a string to a string concatenates the strings
			{
				if (b->type != ELEMENT_STRING) {
					whoops("string concatenation can only be applied to strings");
				}

				String *string_a = a->value;
				String *string_b = b->value;

				// make a new string for the result
				String *result = String_new(string_a->length + string_b->length);

				// iterate through all the characters in both strings and add them to the result string
				for (size_t i = 0; i < result->length; i++) {
					// once the first string is exhausted, move on to the next
					if (i < string_a->length) {
						result->content[i] = string_a->content[i];
					} else {
						result->content[i] = string_b->content[i - string_a->length];
					}
				}

				// make a new Element for the result string and return it
				return make(ELEMENT_STRING, result, heap);
			};
			break;

		default:
			whoops("cannot apply this type to any value");
	}
}

// function to evaluate a branch of the abstract syntax tree
Element *evaluate(Element *branch, Element *ast_root, Stack **call_stack, Stack **scopes_stack, Stack **heap) {
	// get the most recently-added Scope collection from the stack of Scope collections
	Element *scopes = (*scopes_stack)->content[(*scopes_stack)->length - 1];

	switch (branch->type) {
		case ELEMENT_SEQUENCE:
			{
				// each sequence should have its own local scope
				Element *scope = make(ELEMENT_SCOPE, Scope_new(), heap);
				scopes->value = Stack_push(scopes->value, scope);

				Stack *sequence = branch->value;

				// iterate through each statement in the sequence
				for (size_t statement_index = 0; statement_index < sequence->length; statement_index++) {
					Stack *statement = sequence->content[statement_index];

					// get the command name, which is the first element in the statement
					Element *command = statement->content[0];

					// all command names must be plain old words
					if (command->type != ELEMENT_VARIABLE) {
						whoops("command name must not be a value");
					}

					// boolean to indicate whether or not a matching instruction was found
					bool handled = false;

					// the 'do' command just evaluates all its arguments in sequence from left to right
					if (!handled && String_is(command->value, "do") && (handled = true)) {
						// iterate through each argument and evaluate it
						for (size_t i = 1; i < statement->length; i++) {
							evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap);
						}
					}

					// the 'return' command immediately exits the current block and returns the result of an expression
					if (!handled && String_is(command->value, "return") && (handled = true)) {
						if (statement->length != 2) {
							whoops("'return' statement requires exactly 1 argument");
						}

						// evaluate the single expression to determine the result
						Element *result = evaluate(statement->content[1], ast_root, call_stack, scopes_stack, heap);

						// remove the current Sequence's Scope object from the Scope stack
						scopes->value = Stack_pop(scopes->value);

						return result;
					}

					// the 'print' command evaluates all its arguments and prints them to the console from left to right
					if (!handled && String_is(command->value, "print") && (handled = true)) {
						for (size_t i = 1; i < statement->length; i++) {
							Element_print(evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap), 0, false);
						}
					}

					// the 'whoops' command works like the 'print' command but also crashes the program and prints an error string
					if (!handled && String_is(command->value, "whoops") && (handled = true)) {
						for (size_t i = 1; i < statement->length; i++) {
							Element_print(evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap), 0, false);
						}
						whoops("user-defined error");
					}

					// the 'input' command waits for input from the user and then stores it in a variable
					if (!handled && String_is(command->value, "input") && (handled = true)) {
						if (statement->length != 2) {
							whoops("'input' statement requires exactly 1 argument");
						}

						Element *key = statement->content[1];

						// make a temporary buffer to store the input
						char *buffer = NULL;
						size_t allocated_length = 0;

						// the actual length of the user input will be returned by getline, but we'll subtract one because we don't want the newline character at the end
						size_t actual_length = getline(&buffer, &allocated_length, stdin) - 1;

						String *result = String_new(actual_length);

						// move the useful data in the buffer to the result string
						for (size_t i = 0; i < actual_length; i++) {
							result->content[i] = buffer[i];
						}

						// we no longer need the temporary buffer so it can be safely free'd
						free(buffer);

						// make a new String Element for the result and set it to the variable
						set_variable(key, make(ELEMENT_STRING, result, heap), scopes, false);
					}

					// the 'readfile' command reads the contents of a file into a variable
					if (!handled && String_is(command->value, "readfile") && (handled = true)) {
						if (statement->length != 3) {
							whoops("'readfile' command requires exactly 2 arguments");
						}

						Element *key = statement->content[1];

						// evaluate the path argument
						Element *path = evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);
						if (path->type != ELEMENT_STRING) {
							whoops("'readfile' command requires the second argument to be a filepath string");
						}

						String *path_string = path->value;

						// make a temporary char array to store the path so that it can be passed to read_file()
						char path_buffer[path_string->length + 1];

						// copy the string contents to the temporary buffer
						memcpy(&path_buffer, path_string->content, path_string->length);

						// add a null terminator to the path string
						path_buffer[path_string->length] = '\0';

						// read the file
						String *result_string = read_file(path_buffer);

						Element *result;

						// if the file contents can be read, make a String element and store them in it
						// otherwise, the result will be a null element
						if (result_string == NULL) {
							result = make(ELEMENT_NULL, NULL, heap);
						} else {
							result = make(ELEMENT_STRING, result_string, heap);
						}

						set_variable(key, result, scopes, false);
					}

					// the 'writefile' command writes the contents of a String to a file and stores whether or not the operation succeeded in a variable
					if (!handled && String_is(command->value, "writefile") && (handled = true)) {
						if (statement->length != 4) {
							whoops("'writefile' command requires exactly 3 arguments");
						}

						Element *key = statement->content[1];

						// evaluate the new contents for the file
						Element *new_contents = evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);
						if (new_contents->type != ELEMENT_STRING) {
							whoops("'writefile' command requires the first argument to be a string");
						}

						// evaluate the path argument
						Element *path = evaluate(statement->content[3], ast_root, call_stack, scopes_stack, heap);
						if (path->type != ELEMENT_STRING) {
							whoops("'writefile' command requires the second argument to be a filepath string");
						}

						String *path_string = path->value;

						// make a temporary char array to store the path so that it can be passed to read_file()
						char path_buffer[path_string->length + 1];

						// copy the string contents to the temporary buffer
						memcpy(&path_buffer, path_string->content, path_string->length);

						// add a null terminator to the path string
						path_buffer[path_string->length] = '\0';

						// create a new Number to represent the result
						Number *result = Number_new();

						// attempt to write the new contents to the file and update the result number's value accordingly
						result->value_long = write_file(path_buffer, new_contents->value) ? 1 : 0;

						// update the variable to reflect the writing operation's verdict by setting it to a new Number Element representing said verdict
						set_variable(key, make(ELEMENT_NUMBER, result, heap), scopes, false);
					}

					if (!handled && String_is(command->value, "if") && (handled = true)) {
						if (statement->length < 3) {
							whoops("'if' statement requires at least 2 arguments");
						}

						bool result = false;

						// the 'if' statement accepts any number of conditions and actions and evaluates the first action for which the evaluation of the condition is truthy. A trailing action will be evaluated if one is present and if no other actions were evaluated
						for (size_t i = 1; i < statement->length && !result; i += 2) {
							if (i + 1 == statement->length) {
								// if this is the last statement, execute it as the last action
								evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap);
							} else if (Element_is_truthy(evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap))) {
								// if there is a condition and it evaluates to a truthy value, evaluate it and cease further evaluations

								result = true;
								evaluate(statement->content[i + 1], ast_root, call_stack, scopes_stack, heap);
							}
						}
					}

					// the 'while' statement accepts a condition and an action and will execute the action until the condition evaluates to a falsy value
					if (!handled && String_is(command->value, "while") && (handled = true)) {
						if (statement->length != 3) {
							whoops("'while' statement requires exactly 2 arguments");
						}

						// evaluate the condition and check if it's a truthy value before iterating
						while (Element_is_truthy(evaluate(statement->content[1], ast_root, call_stack, scopes_stack, heap))) {
							// evaluate the action
							evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);
						}
					}

					// the 'let' command accepts a key and a value and makes a new variable accordingly
					if (!handled && String_is(command->value, "let") && (handled = true)) {
						if (statement->length != 3) {
							whoops("'let' statement requires exactly 2 arguments");
						}

						Element *key = statement->content[1];

						// evaluate the second argument to find the value to which the variable should be assigned
						Element *value = evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);

						// update the relevant scope with the new mapping
						set_variable(key, value, scopes, true);
					}

					// the 'set' command accepts a key and a value and updates an existing variable accordingly
					if (!handled && String_is(command->value, "set") && (handled = true)) {
						if (statement->length != 3) {
							whoops("'set' statement requires exactly 2 arguments");
						}

						Element *key = statement->content[1];

						// evaluate the second argument to find the value to which the variable should be assigned
						Element *value = evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);

						// update the relevant scope with the new mapping
						set_variable(key, value, scopes, false);
					}

					// the 'mut' command accepts a Scope object, a key expression and a value expression and updates the Scope object accordingly
					if (!handled && String_is(command->value, "mut") && (handled = true)) {
						if (statement->length != 4) {
							whoops("'mut' statement requires exactly 3 arguments");
						}

						// evaluate the first argument to find the subject
						Element *subject = evaluate(statement->content[1], ast_root, call_stack, scopes_stack, heap);

						if (subject->type != ELEMENT_SCOPE) {
							whoops("'mut' statement requires a Scope object as the first argument");
						}

						// evaluate the second argument to find the key
						Element *key = evaluate(statement->content[2], ast_root, call_stack, scopes_stack, heap);

						// evaluate the third argument to find the value
						Element *value = evaluate(statement->content[3], ast_root, call_stack, scopes_stack, heap);

						// update the Scope with the new mapping
						subject->value = Scope_set(subject->value, key, value);
					}

					// the 'edit' command accepts a Scope object, a property name and a value and updates the Scope object accordingly
					if (!handled && String_is(command->value, "edit") && (handled = true)) {
						if (statement->length != 4) {
							whoops("'edit' statement requires exactly 3 arguments");
						}

						// evaluate the first argument to find the subject
						Element *subject = evaluate(statement->content[1], ast_root, call_stack, scopes_stack, heap);

						if (subject->type != ELEMENT_SCOPE) {
							whoops("'mut' statement requires a Scope object as the first argument");
						}

						Element *property_name = statement->content[2];

						// evaluate the third argument to find the value
						Element *value = evaluate(statement->content[3], ast_root, call_stack, scopes_stack, heap);

						// update the Scope with the new mapping
						subject->value = Scope_set(subject->value, property_name, value);
					}

					// if no matching command was found for this statement, it must be an invalid command
					if (!handled) {
						whoops("command not recognised");
					}

					// collect any garbage that may have accumulated over the course of the execution of this statement
					garbage_collect(NULL, ast_root, call_stack, scopes_stack, heap);
				}

				// remove the current Sequence's Scope object from the Scope stack
				scopes->value = Stack_pop(scopes->value);

				// if no value was returned by the sequence, return its own scope
				return scope;
			};
			break;

		case ELEMENT_VARIABLE:
			// if it's a variable name, return its value
			return get_variable(branch, scopes);

		case ELEMENT_OPERATION:
			// handle the behaviours of each operation
			{
				Operation *operation = branch->value;

				switch (operation->type) {
					case OPERATION_APPLICATION:
						// this operation handles the case where two values are juxtaposed
						{
							// evaluate each operand to obtain the actual values we need to operate on
							Element *a = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							Element *b = evaluate(operation->b, ast_root, call_stack, scopes_stack, heap);

							return apply(a, b, ast_root, call_stack, scopes_stack, heap);
						};
						break;

					case OPERATION_EQUALITY:
					case OPERATION_INEQUALITY:
						{
							// evaluate each operand to obtain the actual values we need to operate on
							Element *a = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							Element *b = evaluate(operation->b, ast_root, call_stack, scopes_stack, heap);

							// create a new number for the result
							Number *number = Number_new();

							// check if the two elements are equal or not
							bool result = Element_compare(a, b);

							// if we are checking for inequality, invert the result
							if (operation->type == OPERATION_INEQUALITY) {
								result = !result;
							}

							// set the number value to either 1 (true) or 0 (false) depending on the result
							number->value_long = result ? 1 : 0;

							// make a new Number Element containing the new number
							return make(ELEMENT_NUMBER, number, heap);
						};
						break;

					case OPERATION_ADDITION:
					case OPERATION_SUBTRACTION:
					case OPERATION_MULTIPLICATION:
					case OPERATION_DIVISION:
					case OPERATION_REMAINDER:
					case OPERATION_POW:
					case OPERATION_LT:
					case OPERATION_GT:
					case OPERATION_LTE:
					case OPERATION_GTE:
						{
							// evaluate each operand to obtain the actual values we need to operate on
							Element *a = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							Element *b = evaluate(operation->b, ast_root, call_stack, scopes_stack, heap);

							// throw an error if either one is not a number
							if (a->type != ELEMENT_NUMBER || b->type != ELEMENT_NUMBER) {
								whoops("numeric operations can only be applied to numeric values");
							}

							// operate on the numbers and return a new Number Element containing the result
							return make(ELEMENT_NUMBER, Number_operate(operation->type, a->value, b->value), heap);
						};
						break;

					case OPERATION_SHIFT_LEFT:
					case OPERATION_SHIFT_RIGHT:
					case OPERATION_AND:
					case OPERATION_OR:
					case OPERATION_XOR:
						// handle all the bitwise operations
						{
							// evaluate each operand to obtain the actual values we need to operate on
							Element *a = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							Element *b = evaluate(operation->b, ast_root, call_stack, scopes_stack, heap);

							if (a->type != ELEMENT_NUMBER || b->type != ELEMENT_NUMBER) {
								whoops("bitwise operations may only be applied to integers");
							}

							Number *number_a = a->value;
							Number *number_b = b->value;

							if (number_a->is_double || number_b->is_double) {
								whoops("bitwise operations may only be applied to integers");
							}

							// make a new number to store the result
							Number *result = Number_new();

							// perform the appropriate bitwise operation
							switch (operation->type) {
								case OPERATION_SHIFT_LEFT:
									result->value_long = number_a->value_long << number_b->value_long;
									break;
								case OPERATION_SHIFT_RIGHT:
									result->value_long = number_a->value_long >> number_b->value_long;
									break;
								case OPERATION_AND:
									result->value_long = number_a->value_long & number_b->value_long;
									break;
								case OPERATION_OR:
									result->value_long = number_a->value_long | number_b->value_long;
									break;
								case OPERATION_XOR:
									result->value_long = number_a->value_long ^ number_b->value_long;
									break;
							}

							// make a new Element to store the result and return it
							return make(ELEMENT_NUMBER, result, heap);
						};
						break;

					case OPERATION_SUBL:
					case OPERATION_SUBG:
						// these operators are used to obtain portions of a string based on a number 'n'
						// </ (OPERATION_SUBL) keeps only the first n characters of the string
						// >/ (OPERATION_SUBG) keeps only everything after the first n characters of the string
						{
							// evaluate each operand to obtain the actual values we need to operate on
							Element *a = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							Element *b = evaluate(operation->b, ast_root, call_stack, scopes_stack, heap);

							// make sure that the types line up for this operation
							if (a->type != ELEMENT_STRING || b->type != ELEMENT_NUMBER) {
								whoops("substring operations must be applied to a string and a non-negative integer");
							}

							String *string = a->value;
							Number *slice_index = b->value;

							// ensure that the number supplied is not a negative number or a floating-point value, since these kinds of values are not easily applicable to string slicing
							if (slice_index->is_double || slice_index->value_long < 0) {
								whoops("substring operations must be applied to a string and a non-negative integer");
							}

							// figure out the length of the new string
							// for SUBL, this will either be the length of the string or the slice index, whichever is smaller
							// for SUBG, this will either be the subtraction of the slice index from the length or zero if the slice index is bigger than the length
							size_t length;
							if (operation->type == OPERATION_SUBL) {
								if (slice_index->value_long >= string->length) {
									length = string->length;
								} else {
									length = slice_index->value_long;
								}
							} else {
								if (slice_index->value_long >= string->length) {
									length = 0;
								} else {
									length = string->length - slice_index->value_long;
								}
							}

							String *result = String_new(length);

							// iterate through the characters of the new string and update them to match the relevant characters in the old string
							for (size_t i = 0; i < length; i++) {
								if (operation->type == OPERATION_SUBL) {
									result->content[i] = string->content[i];
								} else {
									result->content[i] = string->content[i + slice_index->value_long];
								}
							}

							// make a new Element to store the result and return it
							return make(ELEMENT_STRING, result, heap);
						};
						break;

					case OPERATION_CLOSURE:
						// this operation creates a new Closure Element that can be called by applying it to a value
						{
							Stack *current_scopes = scopes->value;

							// create a copy of the current Scope collection so that its contents will be preserved until the closure is called
							Element *scopes_copy = make(ELEMENT_SCOPE_COLLECTION, Stack_new(), heap);
							for (size_t i = 0; i < current_scopes->length; i++) {
								scopes_copy->value = Stack_push(scopes_copy->value, current_scopes->content[i]);
							}

							// if a variable name is not specified, don't bother setting it
							// otherwise, use the variable name specified
							Element *variable = operation->a->type == ELEMENT_NULL ? NULL : operation->a;

							return make(ELEMENT_CLOSURE, Closure_new(operation->b, variable, scopes_copy), heap);
						};
						break;

					case OPERATION_ACCESS:
						// this operation does the same thing as applying a value to a Scope, except it applies variable names, similarly to how object properties are accessed in other languages
						{
							Element *subject = evaluate(operation->a, ast_root, call_stack, scopes_stack, heap);
							if (subject->type != ELEMENT_SCOPE) {
								whoops("property access operation can only have a scope as a subject");
							}

							// retrieve the value from the scope, if any
							Element *result = Scope_get(subject->value, operation->b);
							if (result == NULL) {
								// if no result is found, print the property name and an error message
								putchar('\n');
								Element_print(operation->b, 0, true);
								putchar('\n');
								whoops("no such property in this scope");
							}

							return result;
						};
						break;

					default:
						// throw an error if the user uses any operators that haven't been defined yet
						whoops("operator not defined");
				}
			};
			break;

		default:
			return branch;
	}
}

// function to execute a script string
void execute(String *script) {
	// make a new stack to keep track of all the elements that will be stored on the heap
	// this will be useful for garbage collection later
	Stack *heap = Stack_new();

	// construct the list of tokens from the string
	Stack *tokens = tokenise(script, &heap);

	// construct the abstract syntax tree from the token list
	Element *ast_root = construct_tree(tokens, &heap);

	// we no longer have any use for the token list, so it should be freed
	free(tokens);

	// make a stack to keep track of the previous functions so they don't get garbage collected
	Stack *call_stack = Stack_new();

	// make a stack to keep track of the previous sets of scopes so they don't get garbage collected
	Stack *scopes_stack = Stack_new();

	// make the initial collection of scopes
	scopes_stack = Stack_push(scopes_stack, make(ELEMENT_SCOPE_COLLECTION, Stack_new(), &heap));

	// evaluate the syntax tree
	evaluate(ast_root, ast_root, &call_stack, &scopes_stack, &heap);

	// we no longer need these stacks after the evaluation so they can be safely freed
	free(call_stack);
	free(scopes_stack);

	// clean up any leftover garbage indiscriminately
	garbage_collect(NULL, NULL, NULL, NULL, &heap);

	// there should be nothing really left to clean up, so the heap stack is no longer needed and should be freed
	free(heap);
}

int main(int argc, char *argv[]) {
	// make sure that the user has supplied a script file to execute
	if (argc < 2) {
		whoops("a script file must be provided as a command-line argument.");
	}

	// read in the file contents or throw an error if the file cannot be read
	String *script = read_file(argv[1]);
	if (script == NULL) {
		whoops("cannot read this script file");
	}

	// evaluate and execute the script
	execute(script);

	// free the memory that the script uses because we won't need it again
	free(script);

	return 0;
}
