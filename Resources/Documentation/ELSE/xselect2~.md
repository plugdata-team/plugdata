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
  description: spread
  default: 1

flags:
- name: index
  description: sets to indexed mode
- name: circular
  description: sets to circular mode

inlets:
  1st:
  - type: signal
    description: selected channel with crossfade
  nth:
  - type: signal
    description: inlets alternate between input position and spread parameter for each channel

outlets:
  1st:
  - type: signal
    description: crossfaded channels

methods:
  - type: index <float>
    description: 1 sets to indexed mode, 0 to non-indexed mode
  - type: circular <float>
    description: 1 sets to circular mode

draft: false
---

[xselect2~] selects between multiple inputs with equal power crossfade between two adjacent channels.
