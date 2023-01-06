---
title: samphold~
description: sample and hold unit
categories:
- object
see_also: {}
pdcategory: Audio Filters
last_update: '0.39'
inlets:
  1st:
  - type: signal
    description: input to be sampled.
  - type: set <float>
    description: set output value.
  - type: reset <float>
    description: set last value of right inlet.
  - type: reset
    description: set last value of right inlet to infinity (forces output).
  2nd:
  - type: signal
    description: control signal (triggers when smaller than previous)	
outlets:
  1st:
  - type: signal
    description: sampled and held signal.
draft: false
---
The samphold~ object samples its left input whenever its right input decreases in value (as a phasor~ does each period, for example.) Both inputs are audio signals.

The "set" message sets the output value (which gets updated normally afterward.) The "reset" message causes samphold~ to act as if the given value were the most recent value of the control (right) input. Use this, for example, if you reset the incoming phasor but don't want the jump reflected in the output as in the example below to the right. Plain "reset" is equivalent to "reset infinity" which forces the next input to be sampled.