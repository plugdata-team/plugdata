---
title: wrap2~

description: wraps between two values

categories:
 - object
 
pdcategory: ELSE, Signal Math

arguments:
  - type: list
    description: 2 floats — min/max, 1 float — max
    default: -1 and 1
  
inlets:
  1st:
  - type: signal
    description: input values to be wrapped
  2nd:
  - type: float/signal
    description: lowest wrap value
  3rd:
  - type: float/signal
    description: highest wrap value
    
outlets:
  1st:
  - type: signal
    description: wrapped values

draft: false
---

[wrap2~] wraps between a low and high value.
