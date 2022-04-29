---
title: array quantile
description: outputs the specified quantile.
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
  - type: float
    description: quantile (between 0 and 1).
  2nd:
  - type: float
    description: array onset (o is the end of array).
  3rd:
  - type: float
    description: number or points (-1 is the end of array).
  4th:
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
"array quantile" outputs the specified quantile by interpreting the array as a histogram. The output will always be in the range from 0 to "array size - 1". The 0.5 quantile is also known as the median. This generalizes the "array random" function allowing you to use the same source of randomness on several arrays, for example. Negative numbers in the array are silently replaced by zero. Quantiles outside the range 0-1 output the x values at the two extremes of the array (0 and 99 here).
