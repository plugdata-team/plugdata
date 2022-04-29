---
title: pow
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
- description: initialize value of right inlet (default 0).
  type: float
inlets:
  1st:
  - type: bang
    description: output the operation on the previously set values.
  - type: float
    description: value to the left side of operation and output.
  2nd:
  - type: float
    description: value to the right side of operation.
outlets:
  1st:
  - type: float
    description: the result of the operation.
draft: false
---
Pow raises a number on the left inlet to a numeric power (given by the right inlet or argument) - like the signal version, pow has protection against NaNs (they become 0).
