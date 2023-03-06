---
title: flanger~

description: flanger effect

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: rate in Hz
  type: float
  default: 0
- description: depth in ms
  type: float
  default: 0
- description: coefficient
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: flanger input
  2nd:
  - type: float
    description: rate in Hz
  3rd:
  - type: float
    description: depth (ms)
  4th:
  - type: float
    description: coefficient for feedforward/feedback lines

outlets:
  1st:
  - type: signal
    description: flanger output

flags:
  - name: -tri
    description: sets waveform to triangular (default sine)


draft: false
---

[flanger~] is a simple flanger FX object.

