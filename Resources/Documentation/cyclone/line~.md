---
title: line~, cyclone/line~

description: linear ramp generator

categories:
- object
pdcategory: cyclone, Envelopes and LFOs
arguments:
- description: sets an initial value for the signal output
  type: float
  default: 0
inlets:
  1st:
  - type: float
    description: jumps immediately to that value unless duration is set to other than 0 via the second inlet
  - type: list
    description: pairs that specify target value and duration (in ms) to reach it (maximum is 128 target-time pairs). For an odd number of elements, the last element is treated as another pair with 0 ms duration
  2nd:
  - type: float
    description: sets the time for the next float send to the left inlet
outlets:
  1st:
  - type: signal
    description: current value or a ramp towards the target
  2nd:
  - type: bang
    description: a bang is sent when final target is reached

methods:
  - type: pause
    description: pauses the output
  - type: resume
    description: restores the output after being paused
  - type: stop
    description: stops and clears pending target-time parameter triples (but continues outputting its last value)

draft: false
---

[line~] generates signal ramps or envelopes. It takes a target and a duration (in ms) values and generates a ramp from its current value to the target value in that period. Target and duration can be set as a list or in different inlets.

