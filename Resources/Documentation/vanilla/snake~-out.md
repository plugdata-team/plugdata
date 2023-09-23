---
title: snake_out~
description: split multichannel signals into mono signals
categories:
- object
pdcategory: vanilla, Mixing and Routing, Audio I/O
last_update: '0.54'
see_also:
- snake~ out
- inlet~
- clone
- throw~
- send~
arguments:
- description: number of channels
  default: 2
  type: float
inlets:
  1st:
  - type: signals
    description: a multichannel signal to break into mono tracks
outlets:
  nth:
  - type: signal
    description: mono outputs
draft: false
---
ssssssssss
