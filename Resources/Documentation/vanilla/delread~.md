---
title: delread~
description: read from a delay line
categories:
- object
see_also:
- fexpr~
- delwrite~
- delread4~
pdcategory: vanilla, Effects, Buffers
last_update: '0.52'
inlets:
  1st:
  - type: float
    description: delay time in ms
outlets:
  1st:
  - type: signal
    description: delayed signal
arguments:
  - type: symbol
    description: delay line name
  - type: float
    description: initial delay time in ms 
  default: 0

draft: false
---
delread~ and delread4~ objects read from a delay line allocated in a delwrite~ object with the same name. Note that in this help file we're using delay names with "$0" (the patch ID number used to force locality in Pd
