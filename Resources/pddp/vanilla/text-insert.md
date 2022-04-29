---
title: text insert
description: insert a line.
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
- text set
- text
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: 'text name if no flags are given (default: none).'
  type: symbol
- description: 'set line number (default: 0).'
  type: float
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
inlets:
  1st:
  - type: list
    description: a line to insert.
  2nd:
  - type: float
    description: line number to insert.
  3rd:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
draft: false
---

