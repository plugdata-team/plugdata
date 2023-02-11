---
title: table, cyclone/table

description: store and edit a number array
categories:
- object
pdcategory: cyclone, Arrays and Tables
arguments:
- description: sets table name - [table] objects with the same name have the same values
  type: symbol
  default: none
inlets:
  1st:
  - type: bang
    description: same as the quantile message with a random number between 0 and 32768 as an argument (see quantile in [pd All_Messages])
  - type: float
    description: index (x): outputs its value or stores the value from the right inlet (if given) at that index
  - type: list
    description: a pair of index (x) and its value (y): sets `y` to `x`
  2nd:
  - type: float
    description: sets value (y) to be stored at the index sent to left inlet

outlets:
  1st:
  - type: float
    description: index values or else depending on the given message

flags:
  - name: @size <float>
    description: default 128
  - name: @name <symbol>
  - name: @embed <float>
    description: default 0

methods:
  - type: clear
    description: sets all values to 0
  - type: const <float>
    description: fills the table with the given number
  - type: cancel
    description: cancels last right inlet input (so next left input outputs a number)
  - type: dump
    description: outputs the stored numbers on the left outlet
  - type: embed <float>
    description: 1 toggles saving the data as part of the patch - default 0 (don't save)
  - type: flags <f, f>
    description: <1, 0> saves contents with the patch when it's saved, <0, 1> doesn't
  - type: fquantile <float>
    description: sends the address at which the sum of all values up to that address is >= to the the sum of all table values times the <float> (between 0 and 1)
  - type: getbits <list>
    description: 1st float is the address to query, 2nd is starting bit location (0-31 from LSB to MSB) and 3rd is how many bits to the right of starting bit should be sent (as a single decimal integer)
  - type: goto <float>
    description: sets a pointer to the address specified by the number
  - type: inv <float>
    description: finds the first table value which is >= the float and sends its address
  - type: length
    description: outputs the table size
  - type: load
    description: sets to load mode: all numbers received in the left inlet are stored beginning at address 0 until the end (additional numbers are ignored) or taken out of load mode by a normal message
  - type: max
    description: retrieves the maximum stored value
  - type: min
    description: retrieves the minimum stored value
  - type: name <symbol>
    description: sets a table name and is the same as `refer`
  - type: next
    description: sends the value stored at the pointed address on left outlet and sets the pointer to the next address (wraps to 1st address when reaching the end)
  - type: normal
    description: changes from `load mode` to `normal` mode (see `load message`)
  - type: open
    description: opens the editor window (same as double-clicking on it)
  - type: prev
    description: similar to `next` message, but the decreases the pointer address (and wraps to last address when reaching start)
  - type: quantile <int>
    description: sends the address at which the sum of the all values up to that address is >= the the sum of all table values times the <int> divided by 2^15 (32768)
  - type: read <symbol>
    description: opens and reads data from a file specified by the symbol. If no symbol is given, an open dialog is shown
  - type: refer <symbol>
    description: sets the object to read from a named table specified by the symbol
  - type: send <symbol, float>
    description: sends the value stored at the address specified by the float to all [receive] objects with the symbol name name
  - type: set <list>
    description: the first float specifies a starting address and the next numbers specify the values to be stored from that address on
  - type: setbits <list>
    description: changes a stored number's bit values: 1st float is the address, 2nd is the starting bit location (0-31 from LSB to MSB), 3rd is how many from starting bit should be modified and 4th is the value (in decimal or hexadecimal form) to which those bits should be set
  - type: size <float>
    description: sets the number of values in the table (default: 128, indexed from 0 to 127)
  - type: sum
    description: outputs the sum of all values
  - type: wclose
    description: closes the editing window
  - type: write
    description: opens a standard save file dialog for saving it in a text file format

draft: false
---

[table] stores and edits a number array. You can graphically edit it by opening it or double clicking on it. There are also several message methods, check it below.

