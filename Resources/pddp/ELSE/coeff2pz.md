---
title: coeff2pz

description: Biquad coefficients to Poles/Zeros conversion

categories:
- object

pdcategory: Math (Conversion)

arguments: none

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
  1st:
  - type: float
    description: overall gain of the filter response

draft: false
---

[coeff2pz] converts an input of biquad coefficients to of 2 poles/zeros coordinates.