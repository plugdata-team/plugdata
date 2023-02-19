---
title: deltaclip~
description: limit changes between samples
categories:
 - object
pdcategory: cyclone, Signal Math, Effects
arguments:
- type: float
  description: delta minimum - typically negative
  default: 0
- type: float
  description: delta maximum - typically positive
  default: 0
inlets:
  1st:
  - type: signal
    description: any signal whose changes will be clipped
  2nd:
  - type: float/signal
    description: maximum delta in decreasing signal
  3rd:
  - type: float/signal
    description: maximum delta in increasing signal
outlets:
  1st:
  - type: signal
    description: input signal with its change limited by the delta minimum and maximum values

methods:
  - type: reset
    description: sets the last value to "0"

draft: false
---

[deltaclip~] limits the change between samples in an incoming signal. This is also known as 'slew limiting'. It has a negative maximum delta for when the signal decays and a positive maximum delta for when it rises. When they're both 0, the signal doesn't shift. Below we divide by the sample rate to get the max amplitude shift per second instead of per sample.

