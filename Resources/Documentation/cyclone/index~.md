---
title: index~
description: non-interpolating array reader
categories:
 - object
pdcategory: cyclone, Arrays and Tables
arguments:
- type: symbol
  description: name of array
- type: float
  description: array channel (1-64)
  default: 1
inlets:
  1st:
  - type: signal
    description: index to read the array
  2nd:
  - type: float
    description: channel number
outlets:
  1st:
  - type: signal
    description: signal read from array without interpolation

draft: false
---

[index~] reads an array without interpolation. The input signal specifies the table index. It's like [tabread~] but works with multi channel arrays and index values are rounded instead of being truncated.

