---
title: dbtoa~
description: convert dBFS to linear amplitude for signals
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: signal/float
    description: value representing dBFS amplitude
outlets:
  1st:
  - type: signal
    description: corresponding linear amplitude value

draft: false
---

[atodb~] takes any given signal representing a dBFS amplitude value and outputs a signal which is a linear amplitude conversion of the input.

