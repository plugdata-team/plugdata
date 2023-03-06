---
title: drive~

description: soft-clip distortion

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: initial drive value (minimum 0)
  default: 1

inlets:
  1st:
  - type: signal
    description: signal input
  2nd:
  - type: signal/signal
    description: drive factor

outlets:
  1st:
  - type: signal
    description: the distorted signal

flags:
  - name: -mode <float>
    description: sets distortion (0 - default, 1, or 2)

methods:
  - type: mode <float>
    description: sets modes <0, 1, or 2>

draft: false
---

[drive~] simulates an analog "soft clipping" distortion by applying non-linear transfer functions.

