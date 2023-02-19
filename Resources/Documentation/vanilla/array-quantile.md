---
title: array quantile
description: outputs the specified quantile
categories:
- object
pdcategory: vanilla, Arrays and Tables
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
- description: array name if no flags are given 
  type: symbol
  default: none
- description: initial onset 
  type: float
  default: 0
- description: initial number of points
  type: float
  default: -1 (end of array)
inlets:
1st:
- type: float
  description: quantile (between 0 and 1)
2nd:
- type: float
  description: array onset (0 is start of array)
3rd:
- type: float
  description: number of points (-1 is the end of array)
4th:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
outlets:
1st:
- type: float
  description: array's quantile at bangs
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
draft: false
---
outputs the specified quantile
