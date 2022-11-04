---
title: xmod~

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

flags:
- name: -pm
  description: sets to phase modulation (default: fm)
 
inlets:
  1st:
  - type: float/signal
    description: frequency of oscillator 1
    pm - sets to phase modulation
    fm - sets to frequency modulation
    
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

[xmod~] performs cross modulation of two sine oscillators. It can perform either frequency (default) or phase modulation.