---
title: text get
description: read and output a line.
categories:
- object
pdcategory: Misc
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text
- text set
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
- description: 'initial number of fields (default: 1).'
  type: float
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
inlets:
  1st:
  - type: float
    description: specify line number and output (0 for first line).
  2nd:
  - type: float
    description: starting field number (-1 for the whole line).
  3rd:
  - type: float
    description: specify number of fields.
  4th:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
outlets:
  1st:
  - type: list
    description: a line from text or fields from a line.
  2nd:
  - type: float
    description: 'line type: 0 if terminated by a semicolon, 1 if by a comma, or 2
      if the line number was out of range.'
draft: false
---
"text get" reads the nth line from the named text and outputs it, or optionally reads one or more specific fields (atoms) from the line.
