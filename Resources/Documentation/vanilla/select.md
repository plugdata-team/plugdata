---
title: select, sel
description: test for matching numbers or symbols
categories:
- object
pdcategory: vanilla, Mixing and Routing, Triggers and Clocks
last_update: '0.33'
see_also:
- route
arguments:
- description: of floats or symbols to match to
  default: 0
  type: list
inlets:
  1st:
  - type: float/symbol
    description: input to compare to arguments
  2nd:
  - type: float/symbol
    description: if there's one argument, an inlet is created to update it
outlets:
  nth:
  - type: float/symbol
    description: bang if input matches $arg
  2nd: #rightmost
  - type: float/symbol
    description: value if input didn't match
draft: false
---
compare numbers or symbols

