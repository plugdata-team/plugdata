---
title: array get
description: get contents as a list.
categories:
- object
pdcategory: Arrays & Tables
last_update: '0.52'
see_also:
- array
- array define
- array size
- array sum
- array set
- array quantile
- array random
- array max
- array min
arguments:
- description: array name if no flags are given (default none).
  type: symbol
- description: initial onset (default 0).
  type: float
- description: initial number of points (default -1, end of array).
  type: float
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
- description: struct name and field name of element structure.
  flag: -f <symbol, symbol>
inlets:
  1st:
  - type: bang
    description: output the elements of the array.
  - type: float
    description: onset (index to output from, 0 is the start).
  2nd:
  - type: float
    description: number or points to output from onset (-1 is the end of array).
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
"array get" outputs all or a selected range of elements of the array as a list.
