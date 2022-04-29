---
title: array define
description: create an array.
categories:
- object
pdcategory: Arrays & Tables
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
- description: array name (default = internal numbered 'table#').
  type: symbol
- description: size and also xrange (default = 100).
  type: float
flags:
- description: saves/keeps the contents of the array with the patch.
  flag: -k
- description: set minimum and maximum plot range.
  flag: -yrange <float, float>
- description: set x and y graph size.
  flag: -pix <float, float>
inlets:
  1st:
  - type: bang
    description: output a pointer to the scalar containing the array.
  - type: send <symbol>
    description: send pointer to a named receive object.
  - type: other messages
    description: '[array define] send other messages that arrays understand like ''const'',
      ''resize'', etc. For reference, see 2.control.examples ''15.array'' and ''16.more.arrays''.'
outlets:
  1st:
  - type: pointer
    description: a pointer to the scalar containing the array.
draft: false
aliases:
- array
---
"array define" maintains an array and can name it so that other objects can find it (and later should have some alternative, anonymous way to be found).
