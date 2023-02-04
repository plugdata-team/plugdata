---
title: mstosamps~
description: convert milliseconds to samples
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: float/signal
    description: time in milliseconds
outlets:
  1st:
  - type: signal
    description: converted number of samples
  2nd:
  - type: float
    description: converted number of samples

---

[mstosamps~] takes time in milliseconds and converts to a corresponding number of samples (depending on sample rate).

