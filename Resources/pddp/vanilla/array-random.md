---
title: array random
description: array as probabilities.
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
- array set
- array quantile
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
    description: bang to generate a random value.
  - type: seed <float>
    description: sets random seed.
  - type: float
    description: sets onset.
  2nd:
  - type: float
    description: set number of points (-1 is the end of the array).
  3rd:
  - type: symbol
    description: set array name.
  - type: pointer
    description: pointer to the array if -s flag is used.
outlets:
  1st:
  - type: float
    description: random index value from the array.
draft: false
---
"array random" makes a pseudo-random number from 0 to 1 and outputs its quantile (which will therefore have probabilities proportional to the table's values.)
