---
title: '<<'
description: bit twiddling
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
{{< md_include "objects/bitwise-operators.md" >}}
