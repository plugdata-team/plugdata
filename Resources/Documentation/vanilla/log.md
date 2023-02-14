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
- description: initialize value of right inlet 
  default: 0
  type: float
inlets:
  1st:
  - type: bang
    description: trigger calculation and output value
  - type: float
    description: set value on left-hand side and trigger output
  2nd:
  - type: float
    description: set value on right-hand side
outlets:
  1st:
  - type: float
    description: the result of the operation
draft: false
---
As in the signal version log~, log takes a base value via an argument or the right inlet, but it defaults to [e](https://en.wikipedia.org/wiki/E_(mathematical_constant
