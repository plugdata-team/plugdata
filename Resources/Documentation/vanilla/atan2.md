---
title: atan2
description: 2-argument arctangent function
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
The atan2 version takes an (x, y
