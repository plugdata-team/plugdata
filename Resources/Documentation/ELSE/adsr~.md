---
title: adsr~

description: attack/decay/sustain/release gated envelope

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
  - type: float
    description: attack time in ms
    default: 10
  - type: float
    description: decay time in ms
    default: 10
  - type: float
    description: sustain amplitude (ratio to gate value)
    default: 1
  - type: float
    description: release time in ms
    default: 10
  - type: float
    description: curve parameter
    default: 1

flags:
- name: -curve <float>
  description: sets curve parameters
  default: 1
- name: -lin
  description: sets to linear mode
  default: log
- name: -lag
  description: sets to lag filter mode
- name: -rel
  description: sets to immediate release mode

inlets:
  1st:
  - type: signal
    description: gate value
  - type: float
    description: gate value in MIDI velocity range (0-127 is 0-1)
  - type: bang
    description: trigger/retrigger
  2nd:
  - type: signal
    description: signal retrigger via impulses and gates
  3rd:
  - type: float/signal
    description: attack time in ms
  4th:
  - type: float/signal
    description: decay time in ms
  5th:
  - type: float/signal
    description: sustain amplitude (ratio to gate value)
  6th:
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
  - type: curve <float>
    description: sets log curve parameter
  - type: lin
    description: sets to linear mode
  - type: lag
    description: sets to lag filter mode
  - type: rel <float>
    description: nonzero sets to immediate release mode
  - type: gate <float>
    description: control gate values
  - type: retrigger
    description: retrigger at control rate
draft: false
---

[adsr~] is an attack/decay/sustain/release gated envelope. At gate on (transition from 0 to any value), the attack ramp is generated, then a decay ramp goes to the sustain point (specified as a ratio of the gate value). At gate off (transition from any value to 0), [adsr~] goes to 0 at the specified release time in ms.
