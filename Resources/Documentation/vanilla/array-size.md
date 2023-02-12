---
title: array size
description: output or set array size
categories:
- object
pdcategory: vanilla, Arrays and Tables
last_update: '0.52'
see_also:
- array
- array define
- array sum
- array get
- array set
- array quantile
- array random
- array max
- array min
arguments:
- description: array name if no flags are given 
  default: none
  type: symbol
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
inlets:
  1st:
  - type: bang
    description: output the array size
  - type: float
    description: set the array size
  2nd:
  - type: symbol
    description: set array name
  - type: pointer
    description: pointer to the array if '-s' flag is used
outlets:
  1st:
  - type: float
    description: array size
draft: false
---
"array define" maintains an array and can name it so that other objects can find it (and later should have some alternative, anonymous way to be found
