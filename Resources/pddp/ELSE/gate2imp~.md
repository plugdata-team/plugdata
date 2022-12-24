---
title: gate2imp~
description: Gate to impulse

categories:
 - object

pdcategory: General

arguments:

inlets:
  1st:
  - type: signal
    description: gate signal to be converted

outlets:
  1st:
  - type: signal
    description: impulse converted from gate

---

[gate2imp~] converts gates to impulses. It sends an impulse when receiving a gate (0 to non 0 transitions) where the impulse value is the same as the gate.

