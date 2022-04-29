---
title: text tolist
description: convert text to a list.
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
- text
- text fromlist
- text search
- text sequence
arguments:
- description: 'text name if no flags are given (default: none).'
  type: symbol
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
inlets:
  1st:
  - type: bang
    description: output contents as a list.
  2nd:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
outlets:
  1st:
  - type: list
    description: contents of text as a list.
draft: false
---
"text tolist" outputs the entire contents as a list. Semicolons, commas, and dollar signs are output as symbols (and so, if symbols like ", " are encountered, they're escaped with backslashes).
