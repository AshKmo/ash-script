/*
	The ash-script programming language interpreter

	Written by Ashley Kollmorgen, 2025
	https://ashkmo.dedyn.io
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <float.h>

#include "String.h"
#include "Stack.h"

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
	OPERATION_APPLICATION_INFIX,
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
	OPERATION_CONCATENATION,
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
const int OPERATION_PRECEDENCE[] = {0, 1, 1, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8, 9, 9, 10, 11, 12, 13};

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

// function to print Elements to the console
void Element_print(Element *element, int indentation, bool literal) {
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

				for (size_t i = 0; i < scope->length; i++) {
					for (int i = 0; i < indentation + 1; i++) {
						putchar('\t');
					}
					fputs("let ", stdout);
					Element_print(scope->maps[i].key, indentation + 1, false);
					putchar(' ');
					Element_print(scope->maps[i].value, indentation + 1, false);
					putchar(';');
					putchar('\n');
				}

				for (int i = 0; i < indentation; i++) {
					putchar('\t');
				}

				putchar('}');
			};
			break;

		default:
			// if the Element does not have its type listed above, print a placeholder indicating its type
			printf("[WEIRD %d]", element->type);
	}
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
							// '\xGG' evaluates to a character specified by the hexadecimal number 'GG', where 'G' represents any hexadecimal digit

							// the first digit represents the 16s place and the second represents the 1s place
							c = 16 * hex_char(script->content[i + 1]) + hex_char(script->content[i + 2]);
							// we've scanned ahead here, so update the index accordingly
							i += 2;
							break;
					}
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
					};
					break;

				case ELEMENT_OPERATION:
					// operators need to become operation elements
					{
						// make the new empty operation element
						Operation *operation = Operation_new(OPERATION_APPLICATION, NULL, NULL);

						// match the new operator to its operation
						// I apologise for this monstrosity but it's necessary
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
						} else if (String_is(current_value, "..")) {
							operation->type = OPERATION_CONCATENATION;
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
						} else if (String_is(current_value, "$")) {
							operation->type = OPERATION_APPLICATION_INFIX;
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

		current_type = new_type;

		// if the current character is not whitespace and is not the quotes at the ends of a string, add it to the current token
		if (current_type != ELEMENT_NOTHING && (in_string || current_type != ELEMENT_STRING)) {
			current_value = String_append_char(current_value, c);
		}

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

	// iterate through all the possible operators and find the one with the worst (highest) precedence value
	int current_precedence = 0;
	size_t operation_location = 0;
	for (size_t i = start; i < end; i++) {
		Element *current_token = expression->content[i];

		if (current_token->type == ELEMENT_OPERATION) {
			Operation *current_operation = current_token->value;

			// only deal with operators that we haven't handled yet and that have a worse or equal precedence value to the last
			if (current_operation->a == NULL && OPERATION_PRECEDENCE[current_operation->type] >= current_precedence) {
				final_element = current_token;
				final_operation = current_operation;
				current_precedence = OPERATION_PRECEDENCE[current_operation->type];
				operation_location = i;
			}
		}
	}

	// if no operation was found, then this expression must entirely consist of application by juxtaposition
	if (final_operation == NULL) {
		return make(ELEMENT_OPERATION, Operation_new(OPERATION_APPLICATION, operatify(expression, start, end - 1, heap), expression->content[end - 1]), heap);
	}

	// if an operation was found, then process everything to the left of it and use it as the operation's first value
	final_operation->a = operatify(expression, start, operation_location, heap);

	// if an operation was found, then process everything to the right of it and use it as the operation's second value
	final_operation->a = operatify(expression, operation_location + 1, end, heap);

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

// function to recursively mark items as non-garbage
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
	}
}

// function to free an element and its contents
void Element_nuke(Element *element) {
	switch (element->type) {
		case ELEMENT_VARIABLE:
		case ELEMENT_NUMBER:
		case ELEMENT_STRING:
		case ELEMENT_OPERATION:
		case ELEMENT_SCOPE_COLLECTION:
		case ELEMENT_SCOPE:
			free(element->value);
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = element->value;

				// free all the statements in the sequence
				for (size_t y = 0; y < sequence->length; y++) {
					free(sequence->content[y]);
				}

				free(sequence);
			};
			break;
	}

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
			i--;
		}
	}
}

// function to spit out an error and kill the program
void whoops(char *reason) {
	fputs("ERROR: ", stderr);
	fputs(reason, stderr);
	fputc('\n', stderr);
	exit(1);
}

// function to evaluate a branch of the abstract syntax tree
Element *evaluate(Element *branch, Element *ast_root, Stack **call_stack, Stack **scopes_stack, Stack **heap) {
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

					// the "do" command just evaluates all its arguments in sequence from left to right
					if (!handled && String_is(command->value, "do") && (handled = true)) {
						// iterate through each argument and evaluate it
						for (size_t i = 1; i < statement->length; i++) {
							evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap);
						}
					}

					// the "print" command evaluates all its arguments and prints them to the console from left to right
					if (!handled && String_is(command->value, "print") && (handled = true)) {
						for (size_t i = 1; i < statement->length; i++) {
							Element_print(evaluate(statement->content[i], ast_root, call_stack, scopes_stack, heap), 0, false);
						}
					}

					// if no matching command was found for this statement, it must be an invalid command
					if (!handled) {
						whoops("command not recognised");
					}

					// collect any garbage that may have accumulated over the course of the execution of this statement
					garbage_collect(NULL, ast_root, call_stack, scopes_stack, heap);
				}

				// remove the now-redundant scope from the scope stack
				scopes->value = Stack_pop(scopes->value);

				// if no value was returned by the sequence, return its scope
				return scope;
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

// function to return a new String object containing the contents of a file
String *read_file(char *path) {
	// open the script file for reading
	FILE *file_pointer = fopen(path, "rb");

	// seek to the end and get the position after the last character
	fseek(file_pointer, 0, SEEK_END);
	long file_size = ftell(file_pointer);

	// return to the start to read the file
	fseek(file_pointer, 0, SEEK_SET);

	// make a new string to store the script
	String *file_content = String_new(file_size);

	// iterate through each character in the file and shove it in the string
	for (long i = 0; i < file_size; i++) {
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

	execute(script);

	// free the memory that the script uses because we won't need it again
	free(script);

	return 0;
}
