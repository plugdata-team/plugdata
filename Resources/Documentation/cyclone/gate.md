---
title: gate
description: route message to an outlet
categories:
 - object
pdcategory: cyclone, Mixing and Routing
arguments:
- type: float
  description: `n` number of outlets (1 to 100)
  default: 1
- type: float
  description: sets initially open outlet
  default: 0 - none
inlets:
  1st:
  - type: bang
    description: reports the open outlet number on the left outlet
  - type: float
    description: sets outlet number (0 is none)
  2nd:
  - type: anything
    description: message to send through a specified outlet
outlets:
  nth:
  - type: anything
    description: outlet $nth for routing any received message

draft: false
---

[gate] routes a message from the second inlet to one of 'n' specified outlets or none of them.

