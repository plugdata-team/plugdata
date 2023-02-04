---
title: pick

description: pick element from message

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: element number
  type: float
  default: 0, none

inlets:
  1st:
  - type: anything
    description: input message
  2nd:
  - type: float
    description: element number

outlets:
  1st:
  - type: anything
    description: the element from the input message

draft: false
---

[pick] picks an element from an input message according to an element number. Negative values count from last to the first element. If you ask for an element number out of the range, nothing is output.

