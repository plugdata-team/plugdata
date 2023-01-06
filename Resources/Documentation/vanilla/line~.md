---
title: line~
description: audio ramp generator
categories:
- object
pdcategory: General Audio Manipulation
last_update: '0.33'
see_also:
- line
- vline~
inlets:
  1st:
  - type: float
    description: set target value and start ramp.
  - type: stop
    description: stop the ramp.
  2nd:
  - type: float
    description: set next ramp time (cleared when ramp starts).
outlets:
  1st:
  - type: signal
    description: ramp values.
draft: false
---
The line~ object generates linear ramps whose levels and timing are determined by messages you send it. A list of two floats distributes the value over the inlets, as usual in Pd. Note that the right inlet (that sets the ramp time in milliseconds) does not remember old values (unlike every other inlet in Pd). Thus, if you don't priorly specify a time in the right inlet and sent line~ a float, it jumps immediately to the target value.