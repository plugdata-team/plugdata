---
title: atodb
description: convert linear amplitude to dBFS
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: float/list
    description: linear amplitude value(s)
  - type: bang
    description: outputs the last converted float value
outlets:
  1st:
  - type: float/list
    description: converted dBFS amplitude value(s)

methods:
  - type: set <float>
    description: sets the float value to be converted via bang

---

Use [atodb] to convert a linear amplitude value to a deciBel Full Scale (dBFS) equivalent. Negative values convert to -Inf as if the input is "0".

