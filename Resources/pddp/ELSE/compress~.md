---
title: compress~

description:

categories:
- object

pdcategory: DSP (Dynamics)

arguments:
  1st:
  - type: float
    description: threshold in dB
    default: -10
  2nd:
  - type: float
    description: attenuation ratio (default 1)
    default: 1
  3rd:
  - type: float
    description: attack time in ms
    default: 10
  4th:
  - type: float
    description: release time in ms
    default: 10
  5th:
  - type: float
    description: output gain adjustment in dB
    default: 0
  6th:
  - type: float
    description: rms average size in samples
    default: 512

inlets:
  1st:
  - type: signal
    description: input signal
  - type: thresh <float>
    description: sets threshold in dB
  - type: ratio <float>
    description: sets attenuation ratio
  - type: attack <float>
    description: sets attack time in ms
  - type: release <float>
    description: sets release time in ms
  - type: gain <float>
    description: output gain adjustment in dB
  - type: size <float>
    description: rms average size in samples (from 1 to 512)

outlets:
  1st:
  - type: signal
    description: compressed signal
  1st:
  - type: float
    description: reduction in dB

draft: false
---

LONG DESCRIPTION HERE
