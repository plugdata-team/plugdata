---
title: array random
description: array as probabilities
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
  description: get index with a probability weighted by its value
- type: float
  description: sets onset
2nd:
- type: float
  description: set number of points (-1 is the end of the array)
3rd:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
outlets:
1st:
- type: float
  description: weighted random index value from the array at bangs
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -f <symbol, symbol>
  description: struct name and field name of element structure
methods:
- type: seed <float>
  description: sets random seed
draft: false
---
array as weighted probabilities
