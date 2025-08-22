# ash-script
A basic interpreter for a simple programming language called ash-script, written for educational purposes.

This code, along with a proper rendering of this readme, is available on Github at [AshKmo/cos10009-custom](https://github.com/AshKmo/ash-script).

## Compiling
Clone this repository, then make a new directory called 'build' next to this file. Ensure `make` and `gcc` are installed, then run `make` to compile the interpreter. Once compiled, a new binary called 'ash-script' will be placed in the 'build' directory.

## Running
Run `./build/ash-script run <script-file>` to execute any valid ash-script file. For instance, run `./build/ash-script run examples/pi.txt` to execute the Pi calculation example. If you want to execute a script directly, run `./build/ash-script eval <script>`. For example, run `./build/ash-script eval 'print "Hello, world!\n";'` to directly run a Hello World program.

## Language reference
ash-script is an interpreted, dynamically-typed, garbage-collected general-purpose programming language.

### Comments
In ash-script, comments can be written between two square brackets, like so:

```
[ this is a comment ]
```

Anything between and including two enclosing square brackets is considered part of a comment and is not executed as code. Thus, multiline comments are also possible:

```
[
    This is a
    multiline
    comment.
]
```

Comments can be nested:

```
[
    [ this is a comment inside of a comment ]
    print "I'm inside a comment so I won't be printed.\n";
]

print "I will be though.\n";
```

An initial shebang line is also automatically skipped:
```
#!/bin/ash-script run
print "Hello, world!\n";
```

### Data types
ash-script supports a number of different data types. The complete list of all data types are as follows:

#### Null (`()`)
Represents a null value.

#### Number (`4`, `-3`, `3.14`, `-2.7`)
Represents either an integer or a floating-point number. A leading `-` signifies a negative number (in this case, the `-` is not an operator).

**NOTE**: ash-script has no unary operators. The `-` at the start of a negative number is interpreted as part of the number and not as its own operator. If you need to subtract a constant from something, add a space between the `-` and the constant.

#### String (`"Hello, world!"`)
Represents a series of characters. In declaration, the following escape sequences can be used:
- `\n`: newline
- `\r`: carriage return
- `\t`: tab
- `\xHH`: the character with hex value HH. Note that this differs from C in that only one pair of hex digits is read and hence this sequence can only represent a single character

#### Scope (`{let x 3; let "Bob" "a person"; let 0 "the first value";}`)
The current local scope of a sequence becomes, by default, the result of the evaluation of said sequence, allowing for scopes to be manipulated as data. Scopes are ash-script's implementation of dictionaries, arrays and objects.

#### Closure (`x => x * 3`, `() => {print "You called me\n";}`, `x => y => x * y`)
Represents a function that accepts a single argument and that contains the scopes enclosing it. Behaves very similarly to first-class functions in other languages, and is created in a very similar fashion to JavaScript's arrow functions.

### Sequences
Every ash-script script is a sequence of statements that are executed in order. A sequence can also be placed inside an expression by enclosing it in braces (`{}`). Each statement in a sequence consists of a command name and a varying number of arguments, followed by a terminating semicolon:

```
print "Hello, world!\n";
```

In this instance, `print` is the command name and `"Hello, world!\n"` is a string that is treated as an argument when the statement is executed.

The complete list of commands are as follows:

#### `do`
Evaluates all arguments in order from left to right, e.g.

```
do {print "Hello, ";} {print "world!\n";};
```

#### `return`
Accepts one argument, which it evaluates immediately. Ceases execution of the current sequence and causes it to evaluate to the evaluation of its argument.

#### `print`
Prints to the console a text representation of the evaluations of each of its arguments, from left to right.

#### `whoops`
Identical to `print`, but also causes the program to immediately cease execution and crash.

#### `rand`
Accepts a single argument and creates a new variable, named by said argument, containing a random floating-point number with a value n such that 0 <= n < 1.

#### `length`
Accepts two arguments and creates a new variable, named by the first argument, set to an integer representing the length of the string in bytes.

#### `input`
Accepts a single argument and creates a new variable, named by said argument, set to a string containing the user's input, which it receives from the console.

#### `readfile`
Accepts two arguments and creates a new variable, named by the first argument, set to either a null value or the contents of a file, the location of which is described by the evaluation of the second argument, which must be a string.

#### `writefile`
Accepts three arguments and creates a new variable, named by the first argument, set to an integer number 1 if the file at the location described by the evaluation of the second argument (which must be a string) has been successfully updated or created with the contents specified by the evaluation of the third argument. Otherwise, the variable is set to an integer number 0.

#### `if`
Accepts any number of arguments, and iterates through each of them two-by-two. For each pair, the first argument in the pair is evaluated and, if the result is a truthy value, the second argument is evaluated and execution of the statement then ceases. A trailing argument, if specified, is evaluated if this never happens. For example:

```
if (x == 3) {
    print "x is three\n";
} (x == 4) {
    print "x is four\n";
} (x == 5) {
    print "x is five\n";
} {
    print "x is neither three, four, nor five\n";
};
```

#### `while`
Accepts two arguments, the first of which is evaluated and, if the result is a truthy value, the second is also evaluated. This is then repeated until the first argument evaluates to a falsy value.

#### `let`
Accepts two arguments, the first of which names a variable that is set within the current scope to the value specified by the evaluation of the second argument.

#### `set`
Accepts two arguments, the first of which names a variable within an enclosing scope that is set to the value specified by the evaluation of the second argument. The scope is chosen by scanning each scope from nearest to furthest until a matching variable is found. If no scope is found, the variable is created within the local scope.

#### `mut`
Accepts three arguments, the first of which must evaluate to a Scope object that will then be updated such that the key described by the evaluation of the second argument will map to the evaluation of the third argument.

#### `unmap`
Accepts two arguments, the first of which must evaluate to a Scope object that will then be updated such that the key described by the evaluation of the second argument will no longer map to its value.

#### `edit`
Accepts three arguments, the first of which must evaluate to a Scope object that will then be updated such that the key described by the second argument (without evaluation) will map to the evaluation of the third argument.

#### `delete`
Accepts two arguments, the first of which must evaluate to a Scope object that will then be updated such that the key described by the second argument (without evaluation) will no longer map to its value.

#### `keys`
Accepts two arguments. The first argument must evaluate to a Scope from which each key (except property names) will be taken. The second argument must evaluate to a Closure object to which each key will be applied, in order of its addition to the Scope.

#### `values`
Accepts two arguments. The first argument must evaluate to a Scope from which each value will be taken. The second argument must evaluate to a Closure object to which each value will be applied, in order of its addition to the Scope.

### Operations
ash-script also supports a variety of operations:

#### Juxtaposition (`x y`)
This operation evaluates to one of many different things depending on the type of the first element involved:
- Scope: the value in the scope mapped to the key matching the evaluation of `y`, or a Null value if no such key is found
- Closure: the application of the closure to the evaluation of `y`
- String: the concatenation of `x` and `y`, which must both be strings
- Null: the same Null value provided

#### Access (`x . y`)
Evaluates to the value in the scope mapped to the key matching `y` (without evaluation).

#### Equality (`x == y`)
Evaluates to an integer number 1 if the evaluations of `x` and `y` are both of comparable types and are equal in value. Otherwise, it evaluates to an integer number 0. Every type is comparable except for Closure.

#### Inequality (`x != y`)
Same as the equality operator, but with the opposite result.

#### Likeness (`x <>= y`)
Evaluates to an integer number 1 if the evaluations of `x` and `y` are of the same type. Otherwise, it evaluates to an integer number 0.

#### Arithmetic operations (`x + y`, `x - y`, `x * y`, `x / y`, `x % y`, `x ** y`)
Pretty self-explanatory.

**NOTE**: ash-script has no unary operators. The `-` at the start of a negative number is interpreted as part of the number and not as its own operator. If you need to subtract a constant from something, add a space between the `-` and the constant.

#### Comparison operations (`x < y`, `x > y`, `x <= y`, `x >= y`)
Also pretty self-explanatory.

#### Bitwise operations (`x << y`, `x >> y`, `x & y`, `x | y`, `x ^ y`)
Not as self-explanatory, but still very consistent with how C implements them.

#### Logical operations (`x ^^ y`, `x && y`, `x || y`)
Works very similarly to the JavaScript logical operations but with an additional XOR operation (`^^`) that returns an integer number 1 if the truthiness of the two values differ, and 0 otherwise.

#### Substring operations (`</`, `>/`)
These operations require that `x` be a String and `y` be an integer. `x </ y` evaluates to a String containing only the first `y` characters of the String `x`, whilst `x >/ y` evaluates to a String containing the entirety of the String `x` except the first `y` characters.

#### Character value (`x @ y`)
Evaluates to an integer representing the value of the byte in the String `x` at index `y`.

#### Character append (`x @@ y`)
Evaluates to a String containing the contents of the String `x` with the character represented by the integer `y` appended to it.

#### Closure (`x => y`)
Evaluates to a new closure that, when applied to a value, will be able to access said value by the variable name `x` and will evaluate to the evaluation of `y`, which will be evaluated when the closure is applied to a value.
