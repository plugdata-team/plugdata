---
title: zerox~

description: detect zero crossings

categories:
 - object

pdcategory: cyclone, Analysis

arguments:
- type: float
  description: sets impulse values from 0-1
  default: 1

inlets:
  1st:
  - type: signal
    description: signal to be analyzed

outlets:
  1st:
  - type: signal
    description: number of zero crossings per signal block
  2nd:
  - type: signal
    description: a click (impulse) at every zero crossing

draft: true
---

[zerox~] functions as a zero-crossing detector and/or a zero-crossing counter (for transient detection). 
Left outlet outputs a value that corresponds to the number of zero crossings per signal block - so it depends on the block size. Right outlet send an impulse at every zero crossing.
