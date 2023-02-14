---
title: autotune2

description: retune to a close scale step

categories:
- object

pdcategory: ELSE, Tuning

arguments:
- description: scale in cents
  type: list
  default: equal temperament

inlets:
  1st:
  - type: float
    description: pitch value in cents to be retuned
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: float
    description: retuned pitch in cents

methods:
  - type: bypass <float>
    description: non-0 sets to bypass mode

draft: false
---

[autotune2] receives a scale as a list of steps in cents and then retunes incoming cents values to the closest step in the scale.
