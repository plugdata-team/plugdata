---
title: bangbang
description: send a number of bangs
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets the 'n' number of bangs/outlets (maximum 40)
  default: 2
inlets:
  1st:
  - type: anything
    description: a bang or any other message triggers [bangbang]
outlets:
  nth:
  - type: bang
    description: a bang via each outlet in the right to left order

---

When it receives any input, [bangbang] outputs bang messages out of each outlet (in the usual right-to-left order). The number of outlets is determined by the argument.
