---
title: mov.avg~

description: moving average filter

categories:
 - object

pdcategory: ELSE, Analysis, Filters, Signal Math

arguments:
- type: float
  description: sets initial number of samples 
  default: 1

inlets:
  1st:
  - type: signal
    description: the signal to be averaged
  2nd:
  - type: float/signal
    description: number of last samples to apply the average to
 
outlets:
  1st:
  - type: signal
    description: the moving average over the last 'n' samples

flags:
  - name: -size <float>
    description: sets buffer size
  - name: -abs
    description: sets to absolute average mode

methods:
  - type: clear
    description: clears filter's memory
  - type: size <float>
    description: sets new maximum size and clears filter's memory

draft: false
---

[mov.avg~] gives you a signal running/moving average over the last 'n' given samples. This is also a type of lowpass filter.
