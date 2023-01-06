---
title: adsr~

description: Attack/Decay/Sustain/Release gated envelope

categories:
 - object

pdcategory: General

arguments:
  1st:
  - type: float
    description: attack time in ms
    default: 0
  2nd:
  - type: float
    description: decay time in ms
    default: 0
  3rd:
  - type: float
    description: sustain amplitude (ratio to gate value)
    default: 0
  4th:
  - type: float
    description: release time in ms
    default: 0

flags:
- name: -log
  description: sets to log mode (default=linear)

inlets:
  1st:
  - type: float/signal
    description: gate value
  - type: bang
    description: trigger/retrigger
  - type: log <float>
    description: non zero sets to "log" mode, "linear" otherwise
  2nd:
  - type: float/signal
    description: attack time in ms
  3rd:
  - type: float/signal
    description: decay time in ms
  4th:
  - type: float/signal
    description: sustain amplitude (ratio to gate value)
  5th:
  - type: float/signal
    description: release time in ms

outlets:
  1st:
  - type: signal
    description: envelope signal
  2nd:
  - type: float
    description: envelope status (on=1 / off=0)

draft: false
---

[adsr~] is an attack/decay/sustain/release gated envelope. At gate on (transition from 0 to any value), the attack ramp is generated, then a decay ramp goes to the sustain point (specified as a ratio of the gate value). At gate off (transition from any value to 0), [adsr~] goes to 0 at the specified release time in ms.
