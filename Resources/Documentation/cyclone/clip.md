---
title: cyclone/clip
description: constrain values to a given range
categories:
- object
pdcategory: cyclone, Data Math
arguments:
- description: sets minimum
  type: float
  default: 0
- description: sets maximum
  type: float
  default: 0
inlets:
  1st:
  - type: float/list
    description: value(s) to clip
  2nd:
  - type: float
    description: sets minimum number of range
  3rd:
  - type: float
    description: sets maximum number of range
outlets:
  1st:
  - type: float/list
    description: clipped value(s)

methods:
  - type: set <f f>
    description: sets minimum and maximum

draft: false
---

Use [clip] to constrain values from floats or lists between a range specified by a minimum and a maximum value.

