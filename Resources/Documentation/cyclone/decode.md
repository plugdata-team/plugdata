---
title: decode
description: switch an outlet according to input
categories:
 - object
pdcategory: cyclone, Mixing and Routing
arguments:
- type: float
  description: sets the number of outlets
  default: 1
inlets:
  1st:
  - type: float
    description: a number to search for a matching outlet
  - type: bang
    description: resends last output
  2nd:
  - type: float
    description: enable-all switch
  3rd:
  - type: float
    description: disable-all switch
outlets:
  nth:
  - type: float
    description: 1 or 0 based on the input

---

[decode] receives a number and looks for a corresponding outlet (numbered from left to right, starting at 0) to switch it on (output: 1) and the others off (output: 0).

