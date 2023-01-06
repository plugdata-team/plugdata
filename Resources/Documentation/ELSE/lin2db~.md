---
title: lin2db~

description: Convert linear amplitude to dBFS

categories:
- object

pdcategory:

arguments:

inlets:
  1st:
  - type: signal
    description: linear amplitude value

outlets:
  1st:
  - type: signal
    description: converted dBFS amplitude value

draft: false
---

[lin2db~] is a simple abstraction that converts amplitude values from linear to a deciBel Full Scale (dBFS). Conversion expression: dbFS = log10(amp) * 20, see the implementation inside the abstraction.

