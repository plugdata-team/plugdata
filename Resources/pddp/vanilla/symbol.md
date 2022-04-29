---
title: symbol
description: store and recall a symbol
categories:
- object
pdcategory: General
last_update: '0.45'
see_also:
- print
- int
- float
arguments:
- description: 'initially stored symbol 
  default:: empty symbol
.'
  type: symbol
inlets:
  1st:
  - type: anything
    description: converts to symbol,  stores it and outputs it.
  - type: bang
    description: output the stored symbol.
  - type: symbol
    description: stores the symbol received and outputs it.
  2nd:
  - type: symbol
    description: stores the symbol (no output