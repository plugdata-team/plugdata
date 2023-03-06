---
title: unpack
description: get elements of compound messages
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.33'
see_also:
- pack
- trigger
arguments:
- description: atom types: float/symbol/pointer (f/s/p)
  default: f f
  type: list
inlets:
  1st:
  - type: list
    description: a list to be split into atoms
outlets:
  nth:
  - type: float/symbol
    description: a float or a symbol, depending on the argument

draft: false
---
