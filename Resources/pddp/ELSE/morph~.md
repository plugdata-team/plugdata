---
title: morph~

description: Cross synthesis

categories:
- object

pdcategory: DSP

arguments:
- type: float
  description: amplitude cross-fade
  default: 1 - "B"
- type: float
  description: initial phase cross-fade
  default: -1 - "A"

inlets:
  1st:
  - type: signal
    description: signal A
  2nd:
  - type: signal
    description: signal B
  3rd:
  - type: float
    description: amplitudes cross-fading
  4th:
  - type: float
    description: initial phases cross-fading

outlets:
  1st:
  - type: signal
    description: signal morph between A/B

draft: false
---

LONG DESCRIPTION HERE
