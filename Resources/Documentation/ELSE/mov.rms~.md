---
title: mov.rms~

description: moving RMS

categories:
 - object

pdcategory: ELSE, Analysis, Signal Math

arguments:
- type: float
  description: sets number of samples in the window
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
    description: the RMS over the time window

flags:
 - name: -size <float>
   description: sets buffer size
 - name: -db
   description: sets output in dBFS

methods:
  - type: clear
    description: clears buffer's memory
  - type: size <float>
    description: sets new maximum size and clears buffer's memory

draft: false
---

[mov.rms~] gives you a running/moving RMS (Root Mean Square) over a time window of samples. At each sample, the RMS of the last 'n' sample is given.
