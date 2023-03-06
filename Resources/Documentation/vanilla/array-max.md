---
title: array max
description: output maximum value of an array
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
- array quantile
- array random
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
- type: bang
  description: finds maximum
- type: float
  description: onset (index to search from, 0 is the start)
2nd:
- type: float
  description: number or points to search (-1 is the end of array)
3rd:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
outlets:
1st:
- type: float
  description: maximum value
2nd:
- type: float
  description: index of found value
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
draft: false
---
find highest value in the array
