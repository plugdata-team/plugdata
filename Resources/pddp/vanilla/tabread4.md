---
title: tabread4~
description: 4-point-interpolating table lookup for signals. lookup for signals.
categories:
- object
see_also:
- tabwrite~
- tabplay~
- tabread
- tabwrite
- tabsend~
- tabreceive~
- tabosc4~
- soundfiler
pdcategory: Audio Oscillators And Tables
last_update: '0.42'
inlets:
  1st:
  - type: signal
    description: sets table index and output its value with interpolation.
  - type: set <symbol>
    description: set the table name.
  2nd:
  - type: float
    description: sets table onset.
outlets:
  1st:
  - type: signal
    description: value of index input.
arguments:
  - type: symbol
    description: sets table name with the sample. 
draft: false
---
Tabread4~ is used to build samplers and other table lookup algorithms. The interpolation scheme is 4-point polynomial as used in delread4~ and tabosc4~.