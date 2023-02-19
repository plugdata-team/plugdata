---
title: dbtoa
description: convert dBFS to linear amplitude
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: float/list
    description: dBFS amplitude value(s)
  - type: bang
    description: outputs the last converted float value
outlets:
  1st:
  - type: float/list
    description: Linear amplitude value(s)

methods:
  - type: set <float>
    description: sets next float value to be converted via bang

draft: false
---

Use [dbtoa] to convert dB values to corresponding linear amplitude.

