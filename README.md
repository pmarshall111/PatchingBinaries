# Patching Binaries

## Why would you do this?

If you own an application with long build times and you have a client affecting bug, you *may* want to patch the binary to get a fix out quickly.

You probably wouldn't want to do this.

## Tutorial

The simple application in `main.cpp` asks the user for an input and then prints whether it's above or below a hardcoded value of 10.
We're going to patch the hardcoded value of 10 to be something else. Here is what happens when you run the unmodified binary:

```
peter@chronos$ make build
peter@chronos$ ./main.tsk
Enter a number, any number at all:
5
Your number is less than or equal to 10
```

### Steps:

1. Find the address in the binary that corresponds to the line that hardcodes 10
1. Write some new binary to that address

### Finding the address

The first step is to find the address in the binary that corresponds to the line `int threshold = 10;`. This can be done using dwarfdump:

```
peter@chronos$ dwarfdump --print-lines main.tsk
.debug_line: line number info for a single cu
Source lines (from CU-DIE at .debug_info offset 0x0000000c):

            NS new statement, BB new basic block, ET end of text sequence
            PE prologue end, EB epilogue begin
            IS=val ISA number, DI=val discriminator value
<pc>        [lno,col] NS BB ET PE EB IS= DI= uri: "filepath"
0x000014ae  [6620, 3] NS uri: "/home/peter/Documents/personalProjects/binaryPatching/main.cpp"
0x000014c5  [6620,40] NS
0x000014fa  [6621,22] NS
0x000014fc  [6621,22] NS ET
0x000012e9  [   5,12] NS
0x000012f6  [   5,12] NS
0x00001305  [   6,18] NS
0x0000131e  [   6,63] NS
0x00001330  [   7,17] NS
0x0000133c  [   8,17] NS
0x00001352  [   9,19] NS
0x0000136b  [  10, 9] NS
0x00001372  [  11, 5] NS
0x0000137a  [  12,22] NS
0x00001396  [  12,49] NS
0x000013a3  [  12,67] NS DI=0x1
...
```

Here we can see which line numbers correspond to which addresses in the binary. `int threshold = 10;` is line 10 in `main.cpp`, and we can see from the dwarf information that line 10 corresponds to address `0x0000136b`.

We can verify that this address corresponds to line 10 by looking at the assembly for this range. I've used the `-A` argument to grep to ask for 2 lines after the match:

```
peter@chronos$ objdump -d main.tsk | grep -A2 136b
    136b:	c7 45 bc 0a 00 00 00 	movl   $0xa,-0x44(%rbp)
    1372:	8b 45 b8             	mov    -0x48(%rbp),%eax
    1375:	3b 45 bc             	cmp    -0x44(%rbp),%eax
```

Here we can the code at `0x136b` moves 0xa (10 in decimal) into `-0x44(%rbp)` and a later line compares it with `-0x48(%rbp)`. We can see this compare command corresponds to line 11 from the `dwarfdump` information. Line 11 in `main.cpp` compares the user input to the hardcoded value of 10. 

We can verify these addresses correspond to the hardcoded 10 and the user input by loading the binary into gdb and printing the values with the `x/wd` command to print as a 32bit int in decimal:

```
(gdb) x/wd $rbp-0x48
0x7fffffffda58: 5
(gdb) x/wd $rbp-0x44
0x7fffffffda5c: 10
```

### Rewriting the binary

Now we know the command stored at this address, we can change it to move a different number into `-0x44(%rbp)`. Let's substitute 10 (0x0a) for 3 (0x03). For this, we'll use the `dd` utility:

```
peter@chronos$ printf "\xc7\x45\xbc\x03\x00\x00\x00" > hardcode3.bin
peter@chronos$ dd if=hardcode3.bin of=main.tsk seek=$((0x136b)) obs=1 conv=notrunc
0+1 records in
7+0 records out
7 bytes copied, 0.000446964 s, 15.7 kB/s
peter@chronos$ ./main.tsk 
Enter a number, any number at all:
5
Your number is above 3
```

Here, we write the new binary to a file and pass that as an argument to `dd`. Important parameters are `seek` (which tells dd to write at a certain address) and `obs` (which is the block size to seek over). Seek will seek over X objects, so if `obs` is greater than 1 you won't be writing to where you wanted.

We can also verify with `objdump` that `dd` wrote to the correct place:

```
peter@chronos$ objdump -d main.tsk | grep 136b
    136b:	c7 45 bc 03 00 00 00 	movl   $0x3,-0x44(%rbp)
```

The line now has `03` rather than `0a`!

-----------------

## Extra Patch

Another patch we can do is to change a string value. Let's use the `strings` utility to find the location of our `Your number is above` string:

```
peter@chronos$ strings -t x main.tsk | grep above
   2033 Your number is above
```

Now we know the memory address of this string, we can again use the `dd` utility to rewrite the binary:

```
peter@chronos$ printf "Meh, you're above \0" > newString.bin
peter@chronos$ dd if=newString.bin of=main.tsk seek=$((0x2033)) obs=1 conv=notrunc
0+1 records in
19+0 records out
19 bytes copied, 0.000526848 s, 36.1 kB/s
peter@chronos$ ./main.tsk
Enter a number, any number at all:
5
Meh, you're above 3
```