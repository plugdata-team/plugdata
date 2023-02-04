---
title: snapshot~, cyclone/snapshot~
description: convert signal to float
categories:
- object
pdcategory: cyclone, Converters
arguments:
  - description: self clocking interval in ms
    type: float
    default: 0 - stopped
  - description: sample offset within a vector
    type: float
    default: 0
inlets:
  1st:
  - type: signal
    description: signal to convert to float
  - type: bang
    description: outputs a value from the most recent signal block
  - type: float
    description: turns interval-based reports on (non-0) or off (0)
  2nd:
  - type: float
    description: sets time interval in ms
outlets:
  1st:
  - type: float
    description: converted sample from input
flags:
  - name: @active <float>
    description: sets interval based report on <1> / off <0> (default 0)
  - name: @interval <float>
    description: sets interval in ms (default 0 - stopped)

methods:
  - type: start
    description: starts interval-based reporting
  - type: stop
    description: stops interval-based reporting
  - type: offset <f>
    description: sets sample offset within block
  - type: sampleinterval <f>
    description: sets time interval in samples

draft: false
---

[snapshot~] converts signal samples to float when a bang is received or at a given interval in milliseconds.

