---
title: snake~, snake_in~
description: combine mono signals into multichannel signals
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
  nth:
  - type: float/signal
    description: mono input to merge into a multichannel signal
outlets:
  1st:
  - type: signals
    description: multichannel signal
draft: false
---
ssssssssss
