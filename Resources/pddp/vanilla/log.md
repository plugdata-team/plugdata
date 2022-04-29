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
As in the signal version log~, log takes a base value via an argument or the right inlet, but it defaults to [e](https://en.wikipedia.org/wiki/E_(mathematical_constant)).
