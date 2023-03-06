---
title: mix4~

description: 4-channel mixer

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
  3rd:
  - type: signal
    description: incoming signal for channel 3
  4th:
  - type: signal
    description: incoming signal for channel 4

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
  - type: gain3 <float>
    description: sets gain for channel 3
  - type: gain4 <float>
    description: sets gain for channel 4
  - type: pan <float>
    description: sets pan for channel 1
  - type: pan2 <float>
    description: sets pan for channel 2
  - type: pan3 <float>
    description: sets pan for channel 3
  - type: pan4 <float>
    description: sets pan for channel 4
  - type: mode <float>
    description: sets scaling mode: 0 (quartic), 1 (dB) or 2 (linear)

draft: false
---

[mix4~] is a convenient 4 channel mixer abstraction with gain and panning.
