---
title: xmod2~

description: Cross modulation

categories:
 - object
 
pdcategory: General Audio Manipulation

arguments:
  1st:
  - type: float
    description: frequency of oscillator 1 in hertz
    default: 0
  2nd:
  - type: float
    description: modulation index 1
    default: 0
  3rd:
  - type: float
    description: frequency of oscillator 2 in hertz
    default: 0
  4th:
  - type: float
    description: modulation index 2
    default: 0

inlets:
  1st:
  - type: signal
    description: frequency of oscillator 1
  2nd:
  - type: signal
    description: modulation index 1
  3rd:
  - type: signal
    description: frequency of oscillator 2
  4th:
  - type: signal
    description: modulation index 2
    
outlets:
  1st:
  - type: signal
    description: output of oscillator 1
  2nd:
  - type: signal
    description: output of oscillator 2

draft: false
---

[xmod2~] performs is a variant of [xmod~], but only performs frequency modulation (so far) and uses a different and cheap sine waveform that promotes more chaotic and cool sounds.
