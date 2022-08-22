---
title: autotune2

description: Retune to a close scale step

categories:
- object

pdcategory: Audio Filters, General Audio Manipulation

arguments:
- description: scale in cents
  type: list
  default: equal temperament

inlets:
  1st:
  - type: float
    description: pitch value in cents to be retuned
  - type: bypass <float>
    description: non zero sets to bypass mode
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: float
    description: retuned pitch in cents

draft: false
---

[autotune2] receives a scale as a list of steps in cents and then retunes incoming cents values to the closest step in the scale.
