---
title: symbol
description: store and recall a symbol
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.45'
see_also:
- print
- int
- float
arguments:
- description: initially stored symbol 
  default: empty symbol
  type: symbol
inlets:
  1st:
  - type: anything
    description: gets converted to symbol, stored, and output
  - type: bang
    description: output the stored symbol
  - type: symbol
    description: stores the received symbol and outputs it
  2nd:
  - type: symbol
    description: stores the symbol (no output)
outlets:
  1st:
  - type: symbol
    description: the stored symbol

draft: false
---

store a symbol (i.e., string)

