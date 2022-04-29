---
title: array set
description: set contents from a list.
categories:
- object
pdcategory: Arrays & Tables
last_update: '0.52'
see_also:
- array
- array define
- array size
- array sum
- array get
- array quantile
- array random
- array max
- array min
arguments:
- description: array name if no flags are given (default none).
  type: symbol
- description: initial onset (default 0).
  type: float
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
- description: struct name and field name of element structure.
  flag: -f <symbol, symbol>
inlets:
  1st:
  - type: list
    description: list of values to write to array
  2nd:
  - type: float
    description: onset (index to set from, 0 is the start).
  3rd:
  - type: symbol
    description: set array name.
  - type: pointer
    description: pointer to the array if -s flag is used.
outlets:
  1st:
  - type: list
    description: array's elements.
draft: false
---
"array set" sets values of an array from an incoming list, starting from a specified onset (0 by default). The size of the array is not changed - values that would be written past the end of the array are dropped.
