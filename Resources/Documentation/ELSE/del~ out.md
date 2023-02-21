---
title: del~ out

description: delay line output

categories:
 - object

pdcategory: ELSE, Effects, Buffers

arguments:
- type: symbol
  description: sets delay line name
  default: internal name relative to patch
- type: float
  description: delay size in ms or samples
  default: 1 sample

inlets:
  1st:
  - type: float/signal
    description: signal input into the delay line

outlets:
  1st:
  - type: signal
    description: output of a delay line

flags:
  - name: -samps
    description: sets time value to samples (default is ms)
draft: false
---
Read from a delay line. It uses a cubic (4 point) interpolation called spline and considers the buffer to be circular (so index 0 to table size is accepted).

