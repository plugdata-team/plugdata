---
title: sah~

description: sample and hold

categories:
 - object

pdcategory: cyclone, Envelopes and LFOs

arguments:
- type: float
  description: threshold value
  default: 0

inlets:
  1st:
  - type: signal
    description: input to sample and hold
  - type: float
    description: set threshold value
  2nd:
  - type: signal
    description: trigger signal

outlets:
  1st:
  - type: signal
    description: sampled and held signal

draft: true
---

When a trigger signal raises above a given threshold, [sah~] captures a value ("samples") from the input and continually outputs it ("hold") until the trigger signal rises again above the threshold after having dropped below it. This usually synchronizes one signal to the behavior of another.
Sample and hold random values from a noise input, rescaled to 500 - 1100 Hz range:
