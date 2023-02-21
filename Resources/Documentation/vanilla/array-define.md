---
title: array define
description: create an array
categories:
- object
pdcategory: vanilla, Arrays and Tables
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
  default: internal numbered 'table#'
  type: symbol
- description: size and xrange
  default: 100
  type: float
inlets:
1st:
- type: bang
  description: output a pointer to the scalar containing the array
- type: other messages
  description: messages to the array itself (check help file)
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
methods:
- type: send <symbol>
  description: send pointer to a named receive object
draft: false
---
create, store, and/or edit an array
