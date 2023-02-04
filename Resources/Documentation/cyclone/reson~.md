---
title: reson~

description: bandpass resonant filter

categories:
 - object

pdcategory: cyclone, Filters

arguments:
- type: float
  description: initial gain
  default: 0
- type: float
  description: initial center frequency
  default: 0
- type: float
  description: initial Q range
  default: 0.01

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  - type: list
    description: <gain, center frequency,center, Q>
  2nd:
  - type: signal
    description: sets gain
  3rd:
  - type: signal
    description: sets center frequency
  4th:
  - type: signal
    description: sets Q

outlets:
  1st:
  - type: signal
    description: filtered signal

methods:
  - type: clear
    description: clears the internal buffer's memory

draft: true
---

[reson~] is a bandpass resonant filter. All parameters can be set as float or signals.
