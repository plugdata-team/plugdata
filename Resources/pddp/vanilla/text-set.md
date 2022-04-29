---
title: text set
description: replace or add a line
categories:
- object
pdcategory: Misc
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text get
- text
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: 'text name if no flags are given (default: none).'
  type: symbol
- description: 'set field number (default: -1)'
  type: float
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
inlets:
  1st:
  - type: list
    description: a line to add or replace.
  2nd:
  - type: float
    description: line number to replace or add (if greater than the number of lines).
  3rd:
  - type: float
    description: field number to start replacing if negative (or not supplied), replace
      whole line.
  4th:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
draft: false
---
"text set" replaces the nth line with the incoming list. If the number n is greater than the number of lines in the text the new line is added.

If inlet 2 is unset or set to a negative number, the entire line is replaced, but if it is set to 0 or more to specify a starting field, the line is not resized - instead, as many items are replaced as were already in the list. In this case, an out-of-range line number will not cause a new line to be added - instead, the last existing line is modified.
