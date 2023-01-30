---
title: tanh

description: hyperbolic tangent function

categories:
 - object

pdcategory: cyclone, General

arguments:
- type: float
  description: initially stored value
  default: 0

inlets:
  1st:
  - type: bang
    description: output the arc-cosine of stored value
  - type: float
    description: input to arc-cosine function

outlets:
  1st:
  - type: float
    description: the arc-cosine of the input

draft: true
---

[tanh] calculates the hyperbolic tangent function of a given number.
