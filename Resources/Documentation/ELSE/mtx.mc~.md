---
title: mtx.mc~

description: Multuchannel signal routing matrix

categories:
 - object

pdcategory: ELSE, Mixing and Routing

methods:
  - type: ramp <float>
  description: set ramp time in ms
  - type: print
    description: prints state off all cells in pd window
  - type: clear
    description: clears all connections
arguments:
  - type: float
    description: number of inputs (1 to 512)
    default: 1
  - type: float
    description: number of outputs (1 to 512)
    default: 1
  - type: float
    description: ramp time in ms
    default: 10

inlets:
  1st:
  - type: signals
    description: multichannel to route/mix
  - type: list
    description: 3 floats to set: channel input, output and gain

outlets:
  1st:
  - type: signal
    description: routed signals from inlets

draft: false
---

[mtx.mc~] routes a channel signal from a multichannel input to one or more channels of a multichannel output.
