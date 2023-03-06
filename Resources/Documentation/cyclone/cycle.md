---
title: cycle
description: round-robin messages to outlets
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets number of outlets (max 128)
  default: 1
- type: float
  description: mode: Non-0 sets to "event sensitive mode"
  default: 0
inlets:
  1st:
  - type: anything
    description: any message type. Messages with more than one element outputs each element to a different outlet
outlets:
  nth:
  - type: float/symbol
    description: a float or a symbol atom element, depending on the input

methods:
  - type: set <float>
    description: sets the next output outlet (starting at 0)
  - type: thresh <float>
    description: switches output mode

draft: false
---

Each incoming number is sent to the next outlet, wrapping around to the first outlet after the last has been reached, completing a cycle in a "round-robin scheduling" way.

