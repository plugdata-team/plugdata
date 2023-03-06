---
title: eqdiv

description: equal divisions of intervals

categories:
- object

pdcategory: ELSE, Tuning

arguments:
- description: number of equal divisions
  type: float
  default: 12
- description: interval ratio
  type: float
  default: 2

inlets:
  1st:
  - type: bang
    description: generate scale
  - type: float
    description: set number of divisions and generate scale
  2nd:
  - type: float
    description: set interval ratio

outlets:
  1st:
  - type: list
    description: scale in cents or ratio

flags:
  - name: -ratio
    description: sets scale output to ratio instead of cents

draft: false
---

Use [eqdiv] to generate a scale as a list of ratios that equally divide a given interval, the output is in cents. intervals defined as fractions to ratio intervals.

