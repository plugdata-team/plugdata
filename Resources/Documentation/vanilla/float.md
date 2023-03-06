---
title: float, f
description: store and recall a number
categories:
- object
pdcategory: vanilla, Data Math
last_update: '0.48'
see_also:
- int
- value
- send
- symbol
arguments:
- description: initially stored value 
  default: 0
  type: float
inlets:
  1st:
  - type: bang
    description: output the stored value
  - type: float
    description: store and output the value
  - type: list
    description: if first element is a float, stores and outputs it
  - type: symbol
    description: symbols that look like a float are converted, stored and output
  2nd:
  - type: float
    description: store the value (no output)
outlets:
  1st:
  - type: float
    description: the stored value

methods:
  - type: send <symbol>
    description: send the stored value to a [receive] or [value] object that has the same name as the symbol (no output)
draft: false
---

store a (floating point) number.
