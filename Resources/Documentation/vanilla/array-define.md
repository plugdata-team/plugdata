---
title: array, array define
description: Create an array.
categories:
- object
pdcategory: vanilla,  Arrays & Tables
last_update: '0.52'
see_also:
- array
- array size
- array sum
- array get
- array set
- array quantile
- array random
- array max
- array min
- text
- scalar
- list
arguments:
- description: array name 
  default: = internal numbered 'table#'
  type: symbol
- description: size and also xrange
inlets:
1st:
- type: bang
  description: output a pointer to the scalar containing the array
- type: send <symbol>
  description: send pointer to a named receive object
- type: other messages
  description: messages to the array itself (check help file of graphical arrays)
outlets:
1st:
- type: pointer
  description: a pointer to the scalar containing the array at bangs
flags:
- name: -k
  description: saves/keeps the contents of the array with the patch
- name: yrange <float, float>
  description: set minimum and maximum plot range
- name: -pix <float, float>
  description: set x and y graph size
---
create, store, and/or edit an array
