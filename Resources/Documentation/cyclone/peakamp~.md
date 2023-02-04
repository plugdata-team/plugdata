---
title: peakamp~

description: signal's peak amplitude

categories:
 - object

pdcategory: cyclone, Analysis

arguments:
- type: float
  description: sets the internal clock in ms
  default: 0

inlets:
  1st:
  - type: float
    description: signal to measure peak amplitude
  - type: bang
    description: reports a peak value
  2nd:
  - type: float
    description: time interval in ms to report peak values

outlets:
  1st:
  - type: float
    description: reported peak amplitude

draft: true
---

Use the [peakamp~] object to report the absolute value of the peak amplitude of a signal since the last time it was reported. It outputs the peak amplitude when banged or via its own internal clock.
