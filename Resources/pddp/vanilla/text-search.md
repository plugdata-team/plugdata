---
title: text search
description: search for a line.
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
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text
- text sequence
arguments:
- description: 'text name if no flags are given 
  default:: none
.'
  type: symbol
- description: search field number optionally preceded by '>'. '>=', '<', '<=', or
    'near'.
  type: list
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
inlets:
  1st:
  - type: list
    description: search key.
  2nd:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
outlets:
  1st:
  - type: float
    description: found line number or -1 if not found.
draft: false
---
"text search" outputs the line number of the line that best matches a search key. By default it seeks a line whose leading fields match the incoming list.
