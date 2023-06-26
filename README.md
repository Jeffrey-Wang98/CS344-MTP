# CS344-MTP

Multi-Threaded Line Processor in C

Written for Operating Systems CS344

This program is written for Unix systems.

This program utilizes pthreads to process lines from stdin and prints the output every 80 characters. Will not print an output if 80 characters
have not been reached in the last buffer. MTP uses mutex and condition variables to prevent race conditions. 
This program uses this pipeline for processing:

stdin -> Input Thread -> Buffer 1 -> Line Separator Thread -> Buffer 2 -> Plus Sign Thread -> Buffer 3 -> Output Thread -> stdout

First, inputs into stdin, either from user or redirection, will be read by Input Thread. The Input Thread will print each line into Buffer 1.
Line Separator Thread takes lines from Buffer 1 and removes newline characters and writes the output to Buffer 2. Plus Sign Thread takes lines
from Buffer 2 and removes `++` from the input and replaces it with `^`. Then it will output the new line into Buffer 3. The Output Thread will
take lines from Buffer 3 and write them to stdout if the buffer reaches 80 characters. All threads will terminate once an input of `STOP` is
given by stdin.

To run, first download all files and run `make` inside the root directory. After running make, run `./testscript ./mtp`. This should run
the testscript and grades the performance of the program. This should output a final score of 105/105.
