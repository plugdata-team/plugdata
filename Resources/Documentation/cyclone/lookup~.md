---
title: lookup~
description: transfer function lookup table
categories:
 - object
pdcategory: cyclone, Effects, Arrays and Tables
arguments:
- type: symbol
  description: array/table name to be used for lookup
- type: float
  description: initial start point in array/table
  default: 0
- type: float
  description: initial end point in the array/table
  default: 511
inlets:
  1st:
  - type: signal
    description: signal to be used as index
  2nd:
  - type: float/signal
    description: start point of the array
  3rd:
  - type: float/signal
    description: end point of the array
outlets:
  1st:
  - type: signal
    description: output of transfer function lookup (waveshaping)

methods:
  - type: set <symbol>
    description: sets the array/table to be used for lookup

---

[lookup~] uses arrays as transfer functions for waveshaping distortion, in which signal input values (from -1 to 1) are mapped to the table's indexes.

