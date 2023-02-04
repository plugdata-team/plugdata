---
title: minimum
description: compare minimum
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
- type: float
  description: value to compare minimum with
  default: 0
inlets:
  1st:
  - type: bang
    description: resends the most recent output
  - type: float
    description: value to compare minimum with argument or right inlet
  - type: list
    description: minimum number in the list is output and updates argument
  2nd:
  - type: float
    description: value to compare minimum with left inlet (updates argument)
outlets:
  1st:
  - type: float
    description: the minimum (smallest) value of two or more numbers
  2nd:
  - type: float
    description: the index of the minimum value

---

[minimum] outputs the minimum of two or more values in the left outlet. The right outlet outputs the index of the minimum.

