---
title: maximum
description: compare maximum
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
- type: float
  description: value to compare maximum with
  default: 0
inlets:
  1st:
  - type: bang
    description: resends the most recent output
  - type: float
    description: value to compare maximum with argument or right inlet
  - type: list
    description: maximum number in the list is output and updates argument
  2nd:
  - type: float
    description: value to compare maximum with left inlet (updates argument)
outlets:
  1st:
  - type: float
    description: the maximum (greatest) value of two or more numbers
  2nd:
  - type: float
    description: the index of the maximum value

---

[maximum] outputs the maximum of two or more values in the left outlet. The right outlet outputs the index of the maximum.

