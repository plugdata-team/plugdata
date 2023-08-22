---
title: xselect.mc~

description: select channels from multichannel connection with crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: crossfade time in ms
  default: 0 (no crossfade)
- type: float
  description: initially selected channel
  default: none

flags:
- name: -n <float>
  description: number of channels to select 
  default: 1

inlets:
  1st:
  - type: signal
    description: multichannel input to select from
  - type: float
    description: selected input

outlets:
  1st:
  - type: signal
    description: bandlimited step noise

methods:
  - type: time <float>
    description: crossfade time in ms

draft: false
---

[xselect.mc~] selects between multiple channels from a multichannel connection (maximum 512 channels) with equal power crossfading.