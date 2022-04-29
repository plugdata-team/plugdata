---
title: '%'
description: higher math
categories:
- object
pdcategory: Math
see_also:
- +~
- expr
- sin
- log
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
div and mod do integer division, where div outputs the integer quotient and mod outputs the remainder (modulus). In addition the "%" operator (provided for back compatibility) is like "mod" but acts differently for negative inputs (and might act variously depending on CPU design).
