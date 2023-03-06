---
title: coeff2pz

description: biquad coefficients to poles/zeros conversion

categories:
- object

pdcategory: ELSE, Filters, Data Math, Converters

arguments:
inlets:
  1st:
  - type: list
    description: biquad coefficients to convert

outlets:
  1st:
  - type: list
    description: coordinates of 2 Poles
  2nd:
  - type: list
    description: coordinates of 2 Zeros
  3rd:
  - type: float
    description: overall gain of the filter response

draft: false
---

[coeff2pz] converts an input of biquad coefficients to of 2 poles/zeros coordinates.
