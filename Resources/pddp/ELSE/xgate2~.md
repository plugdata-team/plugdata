---
title: xgate2~

description: Route an input with crossfade

categories:
 - object
 
pdcategory: General

arguments:
1st:
- type: float
  description: number of output channels (min 2, max 500)
  default: 2
2nd:
- type: float
  description: <1> sets to indexed mode <0> sets to non-indexed
  default: 1
  
inlets:
  1st:
  - type: signal
    description: channel selection
  - type: index <float>
    description: <1> sets to indexed mode, <0> to non-indexed (default)
  2nd:
  - type: signal
    description: input channel to be routed
    
outlets:
  1st:
  - type: signal
    description: routed outputs with crossfade

draft: false
---

[xgate2~] routes an input signal to 'n' specified outlets with equal power crossfade between two adjacent channels.