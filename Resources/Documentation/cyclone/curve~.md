---
title: curve~
description: curved ramp generator
categories:
 - object
pdcategory: cyclone, Envelopes and LFOs
arguments:
- type: float
  description: initial value
  default: 0
- type: float
  description: curve factor from -1 to 1
  default: 0 - linear
inlets:
  1st:
  - type: list
    description: up to 42 triplets composed of: 1) destination value, 2) time (ms) & 3) curve factor (from -1 to 1)
  - type: float
    description: jumps immediately to that value (unless the duration is set before to other than 0 in the mid inlet)
  2nd:
  - type: float
    description: curve duration in ms - works only if a float is sent into the left inlet after, and it works only once
  3rd:
  - type: float
    description: curve exponential factor parameter (-1 to 1)
outlets:
  1st:
  - type: signal
    description: current value or a ramp towards the target
  2nd:
  - type: bang
    description: a bang is sent when final target from last ramp is reached

methods:
  - type: factor <float>
    description: adjusts curve's exponential factor
  - type: pause
    description: pauses the output
  - type: resume
    description: restores the output after being paused
  - type: stop
    description: stops & clears pending targets (still outputs last value)

---

Similar to [line~], but [curve~] produces curved (non linear) ramp signals. Below, when receiving 2 triples (of destination, time & curve factor), [curve~] generates a simple Attack-Release envelope.

