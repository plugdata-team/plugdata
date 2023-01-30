---
title: compress~

description: compressor

categories:
- object

pdcategory: ELSE, Effects

arguments:
  - type: float
    description: threshold in dB
    default: -10
  - type: float
    description: attenuation ratio (default 1)
    default: 1
  - type: float
    description: attack time in ms
    default: 10
  - type: float
    description: release time in ms
    default: 10
  - type: float
    description: output gain adjustment in dB
    default: 0
  - type: float
    description: RMS average size in samples
    default: 512

inlets:
  1st:
  - type: signal
    description: input signal

outlets:
  1st:
  - type: signal
    description: compressed signal
  2nd:
  - type: float
    description: reduction in dB

methods:
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
    description: RMS average size in samples (from 1 to 512)

draft: false
---

[compress~] is an abstraction that performs compression. It attenuates an input signal above a given threshold.
