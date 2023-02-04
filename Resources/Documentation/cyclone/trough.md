---
title: trough

description: output numbers smaller than the previous

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: initial trough value
  default: 128

inlets:
  1st:
  - type: bang
    description: sends current trough value 
  - type: float
    description: a value to be compared with the trough value
  - type: list
    description: 2nd number sets new 'trough', then 1st element is input
  2nd:
  - type: float
    description: sets new trough value

outlets:
  1st:
  - type: float
    description: the current 'trough' (minimum) value 
  2nd:
  - type: float
    description: 1 (input is a new trough) / 0 (input is not a new trough)
  3rd:
  - type: float
    description: 1 (input is not a new trough) / 0 (input is a new trough)

draft: true
---

[trough] compares the input to a 'trough' (minimum) value. If it's smaller, the input becomes the new trough and is sent out. The middle outlet sends 1 when a new trough is set and 0 otherwise. The right outlet sends 1 when the input is not smaller than the trough and 0 otherwise.
Note: In Pd, [trough] only deals with floats, not ints.
