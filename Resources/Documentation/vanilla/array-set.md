---
title: array set
description: set contents from a list
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
- array quantile
- array random
- array max
- array min
arguments:
- description: array name if no flags are given 
  default: none
  type: symbol
- description: initial onset 
  type: float
  default: 0
inlets:
1st:
- type: list
  description: list of values to write to array
2nd:
- type: float
  description: onset (index to set from, 0 is the start)
3rd:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
draft: false
---
set contents (all or a range) from a list
