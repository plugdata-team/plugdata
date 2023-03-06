---
title: cyclone/clip~
description: constrain a signal to a given range
categories:
- object
pdcategory: cyclone, Signal Math
arguments:
- description: sets minimum value
  type: float
  default: 0
- description: sets maximum value
  type: float
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to be clipped/constrained
  2nd:
  - type: float/signal
    description: minimum constrain value
  3rd:
  - type: float/signal
    description: maximum constrain value
outlets:
  1st:
  - type: signal
    description: clipped signal

draft: false
---

Use the [clip~]* object to constrain input signals between two specified values (minimum and maximum). If the maximum value is less than the minimum, it becomes the minimum value and vice-versa.

