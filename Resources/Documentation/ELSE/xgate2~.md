---
title: xgate2~

description: route an input with crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of output channels (min 2, max 500)
  default: 2
- type: float
  description: <1> sets to indexed mode <0> sets to non-indexed
  default: 1
  
inlets:
  1st:
  - type: signal
    description: channel selection
  2nd:
  - type: signal
    description: input channel to be routed
    
outlets:
  1st:
  - type: signal
    description: routed outputs with crossfade

methods:
  - type: index <float>
    description: <1> — indexed mode, <0> — non-indexed (default)

draft: false
---

[xgate2~] routes an input signal to 'n' specified outlets with equal power crossfade between two adjacent channels.
