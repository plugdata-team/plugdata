---
title: equal

description: check list equality

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: list
  description: list to compare to
  default: empty list

inlets:
  1st:
  - description: input list
    type: list
  - description: input empty list
    type: bang

outlets:
  1st:
  - description: 1 if equal, 0 if not
    type: float

draft: false
---

[equal] compares lists and outputs 1 if they're the same or 0 if they're not. The arguments set the right input.