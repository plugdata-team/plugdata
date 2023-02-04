---
title: plusequals~, +=~, cyclone/+=~

description: signal accumulator

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: sets the initial stored value for the sum
  default: 0

inlets:
  1st:
  - type: signal
    description: accumulated value (outputs only if a signal is connected)
  - type: bang
    description: resets initial stored value to 0
  2nd:
  - type: signal
    description: signal different than 0 resets initial stored value to 0

outlets:
  1st:
  - type: signal
    description: each sample is the sum of all previous input samples

methods:
- type: set <float>
  description: sets the stored value to that number

draft: true
---

[plusequals~] or [+=~] accumulates the received values. Each input sample is added to the previous ones for a running sum. So, started at 0, a signal consisting of 1 values outputs the sequence (1, 2, 3, 4, etc...).
The internal sum can be reset to 0 with a bang (left inlet) or a signal different than 0 in the right inlet (with sample accuracy) - or also set to any value (with the set message).
