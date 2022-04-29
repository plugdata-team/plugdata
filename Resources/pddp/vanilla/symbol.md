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
- description: 'initially stored symbol (default: empty symbol).'
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
    description: stores the symbol (no output).
outlets:
  1st:
  - type: symbol
    description: the stored symbol.
draft: false
---
Store a symbol (i.e., string)

The symbol object stores a symbol,  Pd's data type for handling fixed strings (often filenames,  array names,  send/receive names or the names of other objects in pd).

NOTE: unlike "float",  etc.,  there's no "send" message to forward to another object -- that would conflict with the function of converting arbitrary messages to symbols.
