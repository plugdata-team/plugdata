---
title: trig.delay~

description: trigger delay

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - type: float
    description: delay time in ms
    default: 0

inlets:
  1st:
  - type: signal
    description: trigger signal (negative to positive transition)
  2nd:
  - type: float/signal
    description: delay time in ms

outlets:
  1st:
  - type: signal
    description: delayed trigger

draft: false
---

After receiving a trigger (non-positive to positive transition), [trig.delay~] sends an impulse delayed by a given time in ms. Any triggers which arrive before the delayed output resets and reschedules the delay, in the same way as the [delay] object.
