---
title: tabread4
description: 4-point-interpolating table lookup
pdcategory: vanilla, Arrays and Tables
arguments:
- type: symbol
  description: sets table name with the sample
inlets:
  1st:
  - type: float
    description: sets table index and output its value with interpolation
outlets:
  1st:
  - type: float
    description: value of index input

methods:
- type: set <symbol>
  description: set the table name
draft: false
---
