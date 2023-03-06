---
title: sh~

description: sample & hold

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: float
  description: initial threshold 
  default: 0
- type: float
  description: initially held value
  default: 0
- type: float
  description: modes - 0 (gate) or 1 (trigger)
  default: 0

flags:
- name: -tr
  description: sets to trigger mode

inlets:
  1st:
  - type: float/signal
    description: input to sample and hold
  - type: bang
    description: sample the signal
  2nd:
  - type: signal
    description: gate signal

outlets:
  1st:
  - type: signal
    description: sampled and held signal

methods:
  - type: set <f>
    description: set the held value
  - type: gate
    description: set the gate mode
  - type: trigger
    description: set the trigger mode

draft: false
---

[sh~] samples and holds a value from the left inlet. In "gate" mode (default), values are sampled if the signal in the right inlet is greater than the threshold (0 by default), or held if lesser or equal to it. In "trigger" mode, the value needs to fall below the threshold before it can retrigger [sh~]. A bang also triggers it.
