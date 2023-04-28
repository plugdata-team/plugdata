---
title: log
description: logarithmic function
categories:
- object
pdcategory: vanilla, Data Math
see_also:
- +~
- +
- div
- expr
arguments:
- description: initial base value
  default: e
  type: float
inlets:
  1st:
  - type: bang
    description: trigger calculation and output value
  - type: float
    description: set input value and trigger output
  2nd:
  - type: float
    description: set base value
outlets:
  1st:
  - type: float
    description: the result of the operation
draft: false
---
As in the signal version log~, log takes a base value via an argument or the right inlet, but it defaults to [e](https://en.wikipedia.org/wiki/E_(mathematical_constant
