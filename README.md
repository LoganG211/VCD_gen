# VCD_gen
This repo will allow you convert a binary file to a vcd.

HOW TO:
1. From the Main Directory you can use "make run_c" to build the binary generator, which only creates a binary with 2 different id's of an amount of entries determined by the #define NUM_NODES.
2. It is preferred to use the bin_gen.c to create the binary file then the vcd_gen.py to create the vcd as they are the programs most up to date.
3. vcd_gen.py requires 2 arguments an input .bin file and an output .vcd file. If the user has issues when running the script an example should be printed in the console.
