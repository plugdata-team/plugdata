---
title: tabread4~
description: four-point interpolating table read
categories:
- object
pdcategory: vanilla, Signal Generators, Arrays and Tables
arguments:
- type: symbol
  description: sets table name with the sample
inlets:
  1st:
  - type: signal
    description: sets table index and output its value with interpolation
  2nd:
  - type: float
    description: sets table onset
outlets:
  1st:
  - type: signal
    description: value of index input
methods:
  - type: set <symbol>
    description: set the table name
draft: false
---

