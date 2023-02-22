---
title: array get
description: get contents as a list
categories:
- object
pdcategory: vanilla, Arrays and Tables
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
  description: output the elements of the array
- type: float
  description: onset (index to output from, 0 is the start)
2nd:
- type: float
  description: number or points to output from onset (-1 is the end of array)
3rd:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
outlets:
1st:
- type: list
  description: array's elements for the given range at bangs
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
draft: false
---
get contents (all or a range) as a list
