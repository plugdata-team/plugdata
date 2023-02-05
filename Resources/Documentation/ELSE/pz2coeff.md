---
title: pz2coeff

description: poles/zeros to biquad coefficients conversion

categories:
- object

pdcategory: ELSE, Data Math, Filters, Converters

arguments:

inlets:
  1st:
  - type: bang
    description: calculate the last input values
  - type: list
    description: set coordinates of 2 poles and calculate output
  2nd:
  - type: list
    description: set coordinates of 2 zeros
  3rd:
  - type: float
    description: set overall gain of the filter response

outlets:
  1st:
  - type: list
    description: biquad coefficients as biquad~ takes it

methods:
  - type: poles <list>
    description: set coordinates of 2 poles
  - type: zeros <list>
    description: set coordinates of 2 zeros
  - type: gain <float>
    description: set overall gain of the filter response

draft: false
---

[pz2coeff] converts an input of 2 poles/zeros coordinates to biquad coefficients.

