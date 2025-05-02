# ash-script
A basic interpreter for a simple programming language, written for educational purposes.

## Compiling
First of all, please keep in mind that ash-script has only been tested on a Linux laptop and on Ed Discussion. I've tried to make the code relatively portable, but otherwise there are no guarantees.

Clone this repository, then make a new directory called 'bin' next to this file. Ensure make and gcc are installed, then run `make` to compile the interpreter. Once compiled, a new binary called 'ash-script' will be placed in the 'bin' directory.

## Running
Run `./bin/ash-script <script-file>` to execute any valid ash-script file. For instance, run `./bin/ash-script examples/pi.txt` to execute the Pi calculation example.
