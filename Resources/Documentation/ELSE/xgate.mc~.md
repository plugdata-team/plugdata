---
title: xgate.mc~

description: route a single channel input to a multichannel output

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
  - type: float
    description: number of outlets/channels (min 1, max 500)
    default: 1
  - type: float
    description:  crossfade time in ms 
    default: 0
  - type: float
    description: initially selected channel
    default: 0

inlets:
1st:
  - type: signal(s)
    description: input signal to route
  - type: float
    description: selected outlet (0 is none)
    
outlets:
  1st:
  - type: signal(s)
    description: routed output

methods:
  - type: time <float>
    description: crossfade time in ms

draft: false
---

[xgate.mc~] routes to a channel output in a multichannel connection.