---
title: changed2~

description: detect direction changes

categories:
 - object

pdcategory: ELSE, Analysis, Triggers and Clocks

arguments:
inlets:
  1st:
  - type: signal
    description: signal to detect direction changes

outlets:
  1st:
  - type: signal
    description: impulse at detected direction change
  2nd:
  - type: signal
    description: status (1, -1, or 0)

draft: false
---

[changed2~] sends an impulse whenever an input value changes its direction. The right outlet sends the status information (1 when increasing, -1 when decreasing and 0 for no change).
