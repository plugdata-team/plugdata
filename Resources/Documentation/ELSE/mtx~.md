---
title: mtx~

description: signal routing matrix

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of inputs
  default: 1
- type: float
  description: number of outputs
  default: 1
- type: float
  description: fade time in ms
  default: 10

inlets:
 1st:
  - type: list
    description: inlet, outlet, gain
  nth: 
  - type: signal
    description: signals to route/mix

outlets:
  nth:
  - type: signal
    description: routed signals

methods:
  - type: ramp <float>
    description: sets ramp time in ms
  - type: dump
    description: outputs state of all cells
  - type: clear
    description: clears all connections

draft: false
---

[mtx~] routes signals from any inlets to one or more outlets. If more than one inlet connects to an outlet, the output is the sum of the inlets' signals. Use [mtx.ctl] to control it.
