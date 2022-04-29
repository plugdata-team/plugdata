---
title: atan2
description: trigonometric functions
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
The atan2 version takes an (x, y) pair and gives you an output between -pi and pi, it also takes a bang message in the left inlet to evaluate the operation with the previously set values.

{{< md_include "objects/trigonometric-functions.md" >}}