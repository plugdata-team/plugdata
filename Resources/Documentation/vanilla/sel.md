---
title: sel
description: test for matching numbers or symbols
categories:
- object
pdcategory: General
last_update: '0.33'
see_also:
- select
arguments:
- description: of floats or symbols to match to
  default: 0
  type: list
inlets:
  1st:
  - type: float/symbol
    description: input to compare to arguments.
outlets:
  nth:
  - type: float/symbol
    description: bang if input matches $arg.
  1st:
  - type: float/symbol
    description: value if input didn't match.
