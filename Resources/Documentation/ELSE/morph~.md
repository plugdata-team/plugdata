---
title: morph~

description: cross synthesis

categories:
- object

pdcategory: ELSE, Effects, Mixing and Routing

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

[morph~] allows you to make a spectral crossfade between the amplitudes and phases of two inputs. You can then do cross synthesis and spectral morphing.
