---
title: xselect2~

description: select channel with crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of channels (min 2, max 500)
  default: 2
- type: float
  description: <1> — indexed mode,  <0> — non-indexed
  default: 1
  
inlets:
  1st:
  - type: signal
    description: selected channel with crossfade
  nth:
  - type: signal
    description: secondary inputs are the channels to select from
    
outlets:
  1st:
  - type: signal
    description: crossfaded channels

methods:
  - type: index <float>
    description: <1> - indexed mode, <0> - non-indexed (default)

draft: false
---

[xselect2~] selects between multiple inputs with equal power crossfade between two adjacent channels.
