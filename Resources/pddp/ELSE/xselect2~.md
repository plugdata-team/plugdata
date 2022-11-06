---
title: xselect2~

description: Select channel with crossfade

categories:
 - object
 
pdcategory: Control(Fader/Panning/Routing), General Audio Manipulation

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
  - type: index <f>
    description: <1> - indexed mode, <0> - non-indexed (default)
  nth:
  - type: signal
    description: secondary inputs are the channels to select from
    
outlets:
  1st:
  - type: signal
    description: crossfaded channels

draft: false
---

[xselect2~] selects between multiple inputs with equal power crossfade between two adjacent channels.