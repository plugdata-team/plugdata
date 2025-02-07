---
title: gaterelease~

description: release gate values

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description: release time in ms
  default: (0, no release)

inlets:
  1st:
  - type: signal
    description: gate value
  2nd:
  - type: float
    description: release time in ms

outlets:
  1st:
  - type: signal
    description: released gate

draft: false
---
[gaterelease~] allows you to release one gate in your patch while you still hold another. [gaterelease~] releases a gate value after a given amount of time (specified in ms) after the gate has opened. If the gate is closed before the release time, the object is reset. If the release time is 0 or less, the object is bypassed.
