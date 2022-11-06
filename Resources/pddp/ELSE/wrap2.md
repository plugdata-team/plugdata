---
title: wrap2

description: Wraps between two values

categories:
 - object
 
pdcategory: Math Functions

arguments:
  - type: list
    description: 2 floats — min/max, 1 float — max.
    default: 0 and 1
  
inlets:
  1st:
  - type: float/list
    description: value to be wrapped
  - type: bang
    description: fold or output the last wrapped value (only float)
  - type: set
    description: sets next value to be wrapped via bang (only float)
  2nd:
  - type: float
    description: lowest wrap value
  3rd:
  - type: float
    description: highest wrap value
    
outlets:
  1st:
  - type: float/list
    description:

draft: false
---

[wrap2] wraps between a minimum and maximum value.
