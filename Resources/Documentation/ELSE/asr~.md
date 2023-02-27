---
title: asr~

description: attack/sustain/release gated envelope

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
  - type: float
    description: attack time in ms
    default: 10
  - type: float
    description: release time in  ms
    default: 10

flags:
- name: -lin
  description: sets to linear mode (default=log)

inlets:
  1st:
  - type: float/signal
    description: gate values
  2nd:
  - type: float/signal
    description: attack time in ms
  3rd:
  - type: float/signal
    description: release time in ms

outlets:
  1st:
  - type: signal
    description: envelope signal
  2nd:
  - type: float
    description: envelope status (on=1 / off=0)

methods:
  - type: lin <float>
    description: - non-0 sets to "lin" mode, "log" otherwise

draft: false
---

[asr~] is an attack/sustain/release gated envelope. At gate on (transition from 0 to any value), [asr~] goes from 0 to the gate value at the specified attack time in ms. At gate off (transition from any value to 0) [asr~] goes to 0 at the specified release time in ms.
