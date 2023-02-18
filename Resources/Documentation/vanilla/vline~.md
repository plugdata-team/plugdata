---
title: vline~
description: high-precision audio ramp generator
categories:
- object
pdcategory: vanilla, Signal Generators, Envelopes and LFOs
last_update: '0.33'
see_also:
- line
- line~
inlets:
  1st:
  - type: float
    description: set target value and start ramp
  2nd:
  - type: float
    description: set next ramp time (cleared when ramp starts)
  3rd:
  - type: float
    description: sets delay time
outlets:
  1st:
  - type: signal
    description: ramp values
methods:
  - type: stop
    description: stops the ramp
draft: false
---
The vline~ object, like line~, generates linear ramps whose levels and timing are determined by messages you send it. It takes a target value, a time interval in milliseconds and an initial delay (also in ms). Ramps may start and stop between audio samples, in which case the output is interpolated accordingly.

A list up to three floats distributes the values over the inlets, as usual in Pd. Note that the middle and right inlet (that sets the time and delay) do not remember old values (unlike other inlets in Pd). Thus, if you send vline~ a float without priorly specifying a ramp time and delay and sent, it jumps immediately to the target value. In the same way, a list of two values will not have a delay time if no delay time was priorly set in the right inlet.

Any number of future ramps may be scheduled and vline~ will remember them and execute them in order. They must be specified in increasing order of initial delay however, since a segment cancels all planned segments at any future time.

!!! BUG: vline~ objects inside reblocked subpatches can have slightly incorrect timing !!!
