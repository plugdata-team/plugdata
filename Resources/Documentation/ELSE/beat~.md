---
title: beat~

description: beat detection

categories:
 - object

pdcategory: ELSE, Analysis

arguments:
  - type: float
    description: level threshold
    default: 0.01
  - type: float
    description: set window size
    default: 1024
  - type: float
    description: set hop size
    default: 512

flags:
  - name: -mode <float>
    description: set mode (default: 0)
  - name: -silence <float>
    description: set silence level (default: -70)

inlets:
  1st:
  - type: signal
    description: input to analyze

outlets:
  1st:
  - type: float
    description: detected bpm value

methods:
  - type: mode <float>
    description: set mode (0 to 8)
  - type: thresh <float>
    description: set threshold (0.1 to 1)
  - type: silence <float>
    description: set silence level in dBFS
  - type: window <float>
    description: set analysis window size in samples
  - type: hop <float>
    description: set hop size in samples

draft: false
---

[beat~] takes an input signal and outputs a detected bpm value.

