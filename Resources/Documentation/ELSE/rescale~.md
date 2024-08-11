---
title: rescale~

description: rescale audio

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description: minimum output value
  default: 0
- type: float
  description: maximum output value
  default: 1
- type: float
  description: exponential value
  default: 0, linear

inlets:
  1st:
  - type: signal
    description: value to perform the scaling function on

  2nd:
  - type: signal
    description: minimum output value

  3rd:
  - type: signal
    description: maximum output value

outlets:
  1st:
  - type: signal
    description: the rescaled signal

flags:
- name: -noclip
  description: sets clipping off
- name: -in <float, float>
  description: sets min/max input values
- name: -exp <float>
  description: sets exponential factor
- name: -log
  description: sets to log mode

methods:
- type: exp <float>
  description: sets the exponential factor, -1, 0 or 1 sets to linear
- type: clip <float>
  description: non zero sets clipping on, 0 sets it off
- type: log <float>
  description: non zero sets to log mode

draft: false
---

By default, [rescale~] rescales input values from -1 to 1 into another range of values (0-1 by default).  You can also set to log or exponential factor (0 by default - linear).
