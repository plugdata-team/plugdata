---
title: lin2db

description: convert linear amplitude to dBFS

categories:
- object

pdcategory: ELSE, Data Math

arguments:

inlets:
  1st:
  - type: float
    description: linear amplitude value

outlets:
  1st:
  - type: float
    description: converted dBFS amplitude value

draft: false
---

[lin2db] is a simple abstraction that converts amplitude values from linear to a deciBel Full Scale (dBFS). Conversion expression: dbFS = log10(amp) * 20, see the implementation inside the abstraction.

