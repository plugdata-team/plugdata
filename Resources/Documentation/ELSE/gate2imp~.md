---
title: gate2imp~
description: gate to impulse

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:

inlets:
  1st:
  - type: signal
    description: gate signal to be converted

outlets:
  1st:
  - type: signal
    description: impulse converted from gate

draft: false
---

[gate2imp~] converts gates to impulses. It sends an impulse when receiving a gate (0 to non-0 transitions) where the impulse value is the same as the gate.

