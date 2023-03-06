---
title: wrap
description: wrap a number to range [0, 1)
categories:
- object
pdcategory: vanilla, Data Math
see_also:
- +~
- +
- div
- expr
inlets:
  1st:
  - type: float
    description: input value
outlets:
  1st:
  - type: float
    description: the result of the operation
draft: false
---
The wrap object wraps the input to a value between 0 and 1, including negative numbers (for instance, -0.2 maps to 0.8.)

