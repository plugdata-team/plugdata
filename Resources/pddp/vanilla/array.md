---
title: array
description: general array creation and manipulation
categories:
- object
pdcategory: Arrays & Tables
last_update: '0.52'
see_also:
- array define
- array size
- array sum
- array get
- array set
- array quantile
- array random
- array max
- array min
arguments:
- description: 'sets the function of [array], possible values: define, size, sum,
    get, set, quantile, random, max and min. The default value is "define".'
  type: symbol
- description: array name (default = internal numbered 'table#').
  type: symbol
- description: size and also xrange (default = 100).
  type: float
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
flags:
- description: saves/keeps the contents of the array with the patch.
  flag: -k
- description: set minimum and maximum plot range.
  flag: -yrange <float, float>
- description: set x and y graph size.
  flag: -pix <float, float>
draft: false
---
In Pd an array may be part of a "garray" (a graphical array of numbers) or appear as a slot in a data structure (in which case the elements may be arbitrary data, not necessarily just numbers). The "array" object can define an array (so far just of numbers but maybe later arbitrary data structures) or access an array defined elsewhere to get or change its size, set or read its elements, and so on.
