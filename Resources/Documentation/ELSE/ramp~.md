---
title: ramp~

description: resettable ramp

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: increment value
  default: 1
- type: float
  description: minimum value
  default: 0
- type: float
  description: maximum value
  default: 1
- type: float
  description: start/reset value
  default: minimum value

inlets:
  1st:
  - type: signal
    description: signal trigger (starts ramp from the reset point value)
  - type: bang
    description: control rate trigger (starts ramp from the reset point value)
  2nd:
  - type: float/signal
    description: sets the increment value 
  3rd:
  - type: float/signal
    description: sets the minimum value
  4th:
  - type: float/signal
    description: sets the maximum value

outlets:
  1st:
  - type: signal
    description: the ramp signal
  2nd:
  - type: bang
    description: a bang when ramp is finished

flags:
  - name: -set <float>
    description: sets the reset point value
  - name: -mode <float> 
    description: 0 - wrap, 1 - clip, 2 - reset
  - name: -stop
    description: stops incrementing
  - name: -start
    description: (re)starts incrementing
  - name: -reset
    description: stops and goes back to the reset point

methods:
  - type: mode <float>
    description: 0 - wrap, 1 - clip, 2 - reset
  - type: off
    description: prevents the ramp from automatically starting

draft: false
---

[ramp~] is a resettable linear ramp between a minimum and maximum value. You can trigger it with a bang or with a trigger signal (non-positive to positive transition).
