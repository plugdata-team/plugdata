---
title: peak

description: output numbers greater than the previous

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: initial 'peak' value 
  default: 0

inlets:
  1st:
  - type: bang
    description: sends current 'peak' value to the left outlet
  - type: float
    description: a value to be compared with the 'peak' value
  - type: list
    description: 2nd number sets new 'peak', then 1st element is input
  2nd:
  - type: float
    description: set new 'peak' value

outlets:
  1st:
  - type: float
    description: the current 'peak' (minimum) value
  2nd:
  - type: float
    description: 1 (input is a new peak) / 0 (input is not a new peak)
  3rd:
  - type: float
    description: 1 (input is not a new peak) / 0 (input is a new peak)

draft: true
---

[peak] compares the input to a 'peak' (maximum) value. If it's greater, the input becomes the new peak and is sent out. The middle outlet sends 1 when a new peak is set and 0 otherwise. The right outlet sends 1 when the input is not greater than the peak and 0 otherwise.
Note: In Pd, [peak] only deals with floats, not ints.
