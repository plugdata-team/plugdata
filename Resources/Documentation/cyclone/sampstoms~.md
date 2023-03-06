---
title: sampstoms~

description: convert samples to milliseconds

categories:
 - object

pdcategory: cyclone, Converters

arguments: (none)

inlets:
  1st:
  - type: float/signal
    description: number of samples

outlets:
  1st:
  - type: signal
    description: converted duration in ms
  2nd:
  - type: float
    description: converted duration in ms

draft: true
---

Use the [sampstoms~] object to convert a time value in samples to milliseconds (depending on the sample rate).
[sampstoms~] works with floats and signal. A signal input converts only to the signal outlet, but a float input converts to both float and signal output. When a signal input is present, the float input is ignored.
