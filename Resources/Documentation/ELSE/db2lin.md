---
title: db2lin

description: convert dBFS to linear amplitude

categories:
- object

pdcategory:

arguments:
- description: minimum value (can be as low as "-inf")
  type: float
  default: -100

inlets:
  1st:
  - type: float
    description: dBFS amplitude value

outlets:
  1st:
  - type: float
    description: converted linear amplitude value

draft: false
---

[db2lin] converts amplitude values from deciBel Full Scale (dBFS) to linear. By default, a minimum value of -100 becomes zero - in theory, sero is minus infinity and an argument sets another minimum dB value to correspond to minus ininity. Conversion expression is: amp = pow(10, dBFS / 20).

