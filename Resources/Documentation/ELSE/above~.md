---
title: above~

description: threshold detection

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- description: initial threshold value
  type: float
  default: 0

inlets:
  1st:
  - type: float/signal
    description: input signal
  2nd:
  - type: float/signal
    description: threshold value

outlets:
  1st:
  - type: signal
    description: when left inlet value rises above the threshold
  2nd:
  - type: signal
    description: when left inlet falls back below the threshold

draft: false
---

[above~] sends an impulse to the left outlet when the left inlet is above the threshold value. Conversely, it outputs an impulse to its right outlet when the left inlet falls back to the threshold value or below it. The threshold value can be set via the argument or the 2nd inlet.
