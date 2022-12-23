---
title: log
description: math functions
categories:
- object
pdcategory: Math
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
    description: Trigger calculation and output value
  - type: float
    description: Set value on left-hand side and trigger output
  2nd:
  - type: float
    description: Set value on right-hand side
outlets:
  1st:
  - type: float
    description: The result of the operation.
draft: false
---
As in the signal version log~, log takes a base value via an argument or the right inlet, but it defaults to [e](https://en.wikipedia.org/wiki/E_(mathematical_constant