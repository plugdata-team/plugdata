---
title: mix2~

description: 2-channel mixer

categories:
- object

pdcategory: ELSE, UI, Mixing and Routing

arguments:
- type: float
  description: scaling mode:0 (quartic), 1 (dB) or 2 (linear)
  default: 0

inlets:
  1st:
  - type: signal
    description: incoming signal for channel 1
  2nd:
  - type: signal
    description: incoming signal for channel 2

outlets:
  1st:
  - type: signal
    description: left channel signal
  2nd:
  - type: signal
    description: right channel signal

methods:
  - type: gain <float>
    description: sets gain for channel 1
  - type: gain2 <float>
    description: sets gain for channel 2
  - type: pan <float>
    description: sets pan for channel 1
  - type: pan2 <float>
    description: sets pan for channel 2
  - type: mode <float>
    description: sets scaling mode: 0 (quartic), 1 (dB) or 2 (linear)

draft: false
---

[mix2~] is a convenient 2 channel mixer abstraction with gain and panning.
