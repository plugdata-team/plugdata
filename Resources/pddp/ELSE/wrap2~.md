---
title: wrap2~

description: Wraps between two values

categories:
 - object
 
pdcategory: General

arguments:
  1st:
  - type: list
    description: 2 floats set minimum and maximum. 1 float sets maximum value (and lowest value is set to 0).
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
