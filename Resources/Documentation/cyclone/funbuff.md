---
title: funbuff
description: stores pairs of integers
categories:
 - object
pdcategory: cyclone, Buffers, Data Management
arguments:
- type: symbol
  description: sets a file name to load in the buffer
inlets:
  1st:
  - type: bang
    description: prints object's information in the Pd main window
  - type: float
    description: x value of a x/y pair - saves the x/y pair if a y value was sent before to the right inlet, outputs a y value otherwise
  - type: list
    description: two elements stores a x/y number pair in the buffer
  2nd:
  - type: float
    description: y value of an x/y pair
outlets:
  1st:
  - type: float
    description: the y value of a pair
  2nd:
  - type: float
    description: on a "next" message, the difference with the previous x value
  3rd:
  - type: bang
    description: when the end of the buffer is reached

methods:
  - type: set <list>
    description: sets x/y pairs in the buffer (more than one pair is allowed)
  - type: clear
    description: erases all pairs from the buffer
  - type: select <f, f>
    description: selects a number of x/y pairs (specified by the 2nd float) starting from an index (specified by the 1st float) - for copy/cut/paste
  - type: copy
    description: copies the selected x/y pair to the clipboard
  - type: cut
    description: copies the selected x/y pair to the clipboard and deletes it
  - type: paste
    description: adds the cut or copied x/y pairs
  - type: undo
    description: undoes the latest cut or paste operation
  - type: delete <f, f>
    description: if one float is given, deletes a pair for a matching 'x' value, if two floats are given, deletes a matching x/y pair
  - type: dump
    description: dumps all pairs to the middle (y value) and left (x value) outlets
  - type: embed <float>
    description: non-0 value saves the contents of the buffer when the patch is saved
  - type: goto <float>
    description: sets the buffer pointer to the specified <float> buffer element
  - type: next
    description: outputs the y value of the buffer element and goes to next pointer
  - type: find <float>
    description: finds the 'x' values which have a 'y' that matches the given number
  - type: interp <float>
    description: if a given 'x' doesn't exist, interpolates the 'y' value from 2 neighboring x/y pair
  - type: interptab <f, s>
    description: same as interp, but uses the table specified by the symbol <s>
  - type: min
    description: sends the lowest 'y' value in the buffer to the left outlet
  - type: max
    description: sends the highest 'y' value in the buffer to the left outlet
  - type: print
    description: prints status information in Pd main window
  - type: read <symbol>
    description: reads buffer contents from the file name specified by the symbol. If no symbol is given, a file open box is shown
  - type: write <symbol>
    description: writes buffer contents to the file name specified by the symbol. If no symbol is given, a file open box is shown

draft: false
---

[funbuff] stores, manages and recalls x/y pairs of integers.

