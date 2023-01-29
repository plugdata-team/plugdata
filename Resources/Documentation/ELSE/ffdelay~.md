---
title: ffdelay~
description: feed-forward delay line

categories:
 - object

pdcategory: Effects

arguments:
- type: float
  description: initial/maximum delay time
  default: 0

inlets:
  1st:
  - type: signal
    description: signal input into the delay line
  2nd:
  - type: float/signal
    description: delay time

outlets:
  1st:
  - type: signal
    description: the feed forward delayed signal

flags:
  - name: -size <float>
    description: sets delay size, which defines the delay's maximum time (default 1000 ms or argument's value if given)
  - name: -samps
    description: sets delay time unit to "samples" (default is ms)

methods:
  - type: size <float>
    description: changes the delay size
  - type: freeze <float>
    description: non zero freezes, zero unfreezes
  - type: clear
    description: clears the delay buffer

---

[ffdelay~] is simple feed forward delay with interpolation.

