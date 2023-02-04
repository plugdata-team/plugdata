---
title: gate~
description: route a signal to `n` outlets
categories:
 - object
pdcategory: cyclone, Mixing and Routing
arguments:
- type: float
  description: 'n' number of outlets (1 to 500)
  default: 1
- type: float
  description: initial outlet to route to
  default: 0 - none
inlets:
  1st:
  - type: float/signal
    description: outlet number to route to - values are truncated to integers and clipped from 0 to number of outlets
  2nd:
  - type: signal
    description: signal to be routed
outlets:
  nth:
  - type: signal
    description: outlet $nth for routing input signal

---

[gate~] routes an input signal from the second inlet to one of 'n' specified outlets or none of them. If an outlet is not selected, it outputs zero values.

