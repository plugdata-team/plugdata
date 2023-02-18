---
title: hv.reverb

description: reverb effect unit

categories:
- object

pdcategory: heavylib, Effects

arguments:

inlets:
  1st:
  - type: signal
    description: input signal (mono in, stereo out)
  2nd:
  - type: feedback <float>
    description: feedback amount (0 to 100)
  - type: dry-gain <float>
    description: dry signal gain in dB
  - type: wet-gain <float>
    description: wet signal gain in dB
  - type: lowcut <float>
    description: frequency of lowcut filter in Hz
  - type: highcut <float>
    description: frequency of highcut filter in Hz
  - type: crossfreq <float>
    description: crossover freq in Hz
  - type: damp <float>
    description: damping amount (0 to 100)
  - type: predelay_ms <float>
    description: predelay amount in ms

outlets:
  1st:
  - type: signal
    description: left channel
  2nd:
  - type: signal
    description: right channel

draft: false
---

