---
title: div
description: Divide numbers
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
div and mod do integer division, where div outputs the integer quotient and mod outputs the remainder (modulus