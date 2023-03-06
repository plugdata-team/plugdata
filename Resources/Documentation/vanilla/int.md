---
title: int, i
description: truncate floats and store an integer
categories:
- object
pdcategory: vanilla, Data Math
last_update: '0.48'
see_also:
- float
- value
- send
arguments:
- description: initially stored value 
  default: 0
  type: float
inlets:
  1st:
  - type: bang
    description: output the stored value
  - type: float
    description: store and output truncated value
  - type: list
    description: if first element is float, stores and outputs it
  2nd:
  - type: float
    description: store the value, non-integers are truncated (no output)
outlets:
  1st:
  - type: float
    description: the stored integer value

methods:
  - type: send <symbol>
    description: send the stored value to a [receive] or [value] object that has the same name as the symbol (no output)
draft: false
---
