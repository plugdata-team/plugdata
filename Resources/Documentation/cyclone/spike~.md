---
title: spike~

description: report zero to non-0 transition intervals

categories:
 - object

pdcategory: cyclone, Analysis

arguments: (none)

inlets:
  1st:
  - type: signal
    description: input signal to detect transitions
  - type: bang
    description: restarts the countdown of the refractory period
  2nd:
  - type: float
    description: set refractory period, the maximal reporting rate

outlets:
  1st:
  - type: float
    description: interval in ms since last 0 to non-0 transition

draft: true
---

[spike~] reports intervals of zero to non-0 transitions from the signal input. The refractory period is set at the second inlet, which defines how soon after detecting a transition the spike~ object will report the next instance.
