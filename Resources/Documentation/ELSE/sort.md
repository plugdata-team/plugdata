---
title: sort

description: list sort

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: >= 0 — ascending, descending otherwise
  default: 0

inlets:
  1st:
  - type: anything
    description: a message to sort
  - type: bang
    description: outputs the last incoming message sorted
  2nd:
  - type: float
    description: order (negative is descending, ascending otherwise)

outlets:
  1st:
  - type: list
    description: sorted list
  2nd:
  - type: list
    description: the sorted list as indexes of the original input

draft: false
---

[sort] sorts messages in ascending or descending order. A bang outputs the last incoming message. If you change the sorting order you can use the bang message to change the sorting direction of the last incoming message. Note upper case letters are sorted before lower case letters.
