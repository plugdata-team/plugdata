---
title: cents2ratio

description: Cents/Rational conversion

categories: 
 - object

pdcategory: Math (Conversion)

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
  - type: set
    description: sets next value to be converted via bang (only float)

outlets:
  1st:
  - type: float/list
    description: converted ratio value(s)

draft: false
---

Use [cents2ratio] to convert intervals in cents to interval as rational numbers (expressed as floating point decimals). The conversion formula is;
rational = pow(2, (cents/1200))