---
title: select
description: test for matching numbers or symbols
categories:
- object
pdcategory: General
last_update: '0.33'
see_also:
- route
arguments:
- description: of floats or symbols to match to (default 0).
  type: list
inlets:
  1st:
  - type: float/symbol
    description: input to compare to arguments.
  2nd:
  - type: float/symbol
    description: if there's one argument,  an inlet is created to update it.
outlets:
  'n: (depends on the number of arguments)':
  - type: bang
    description: when the input matches an argument that corresponds to the outlet.
  rightmost:
  - type: float/symbol
    description: when input doesn't match the arguments,  it is passed here.
draft: false
aliases:
- sel
---

