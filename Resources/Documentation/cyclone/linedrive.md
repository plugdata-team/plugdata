---
title: linedrive
description: exponential scaler for [line~]
categories:
 - object
pdcategory: cyclone, Envelopes and LFOs
arguments:
- type: float
  description: input range
  default: 0
- type: float
  description: output range
  default: 0
- type: float
  description: exponential factor (minimum is 1)
  default: 1.01
- type: float
  description: time value for line objects
  default: 0
inlets:
  1st:
  - type: float
    description: input value, to be mapped to output range
  2nd:
  - type: float
    description: time value for line objects
outlets:
  1st:
  - type: list
    description: scaled number and time

---

[linedrive] scales numbers from one range to another with an exponential curve. First argument is the input range (127 means from -127 to 127), second is output range (1 is from -1 to 1), third is exponential factor and fourth is the time (to be used with line~).

