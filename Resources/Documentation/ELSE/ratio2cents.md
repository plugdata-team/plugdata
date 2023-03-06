---
title: ratio2cents

description: rational/Cents conversion

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: initial rational value
  default: 1

inlets:
  1st:
  - type: float/list
    description: rational value(s)
  - type: bang
    description: convert or output the last converted value (only float)

outlets:
  1st:
  - type: float/list
    description: converted cents value(s)

methods:
- name: set
  description: sets next value to be converted via bang (only float)

draft: false
---

Use [ratio2cents] to convert intervals described as rational numbers (expressed as float point decimal values) to intervals in cents. The conversion formula is;
cents = log2(rational) * 1200
