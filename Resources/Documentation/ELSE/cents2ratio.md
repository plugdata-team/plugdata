---
title: cents2ratio

description: cents/rational conversion

categories: 
 - object

pdcategory: ELSE, Tuning, Converters

arguments:
- type: float
  description: initial cents value
  default: 0

inlets:
  1st:
  - type: float/list
    description: cents value(s)
  - type: bang
    description: convert or output the last converted value (only float)

outlets:
  1st:
  - type: float/list
    description: converted ratio value(s)

methods:
  - type: set <float>
    description: sets next value to be converted via bang (only float)

draft: false
---

Use [cents2ratio] to convert intervals in cents to interval as rational numbers (expressed as floating point decimals). The conversion formula is;
rational = pow(2, (cents/1200))
