---
title: allpass~
description: allpass filter
categories:
 - object
pdcategory: cyclone, Filters
arguments:
- type: float
  description: maximum delay time in ms
  default: 10
- type: float
  description: delay time in ms
  default: 0
- type: float
  description: gain coefficient
  default: 0
inlets:
  1st:
  - type: signal
    description: signal input to filter
  - type: list
    description: updates all 3 arguments
  2nd:
  - type: float/signal
    description: delay time (im ms)
  3rd:
  - type: float/signal
    description: gain coefficient
outlets:
  1st:
  - type: signal
    description: the filtered signal

methods:
  - type: clear
    description: clears the buffer

---

Use [allpass~] for filtering and delay effects. The All Pass filter passes all frequencies without altering the gain - but changes the frequencies' phase.

