---
title: noisegate~

description: noise gate

categories:
- object

pdcategory: ELSE, Effects

arguments:
  - description: threshold in dBFS
    type: float
    default: -100
  - description: attack time in ms
    type: float
    default: 10
  - description: release time in ms
    type: float
    default: 10

inlets:
  1st:
  - type: signal
    description: signal to be gated
  2nd:
  - type: float
    description: threshold in dBFS

outlets:
  1st:
  - type: signal
    description: gated signal

flags:
- name: -size <float>
  description: sets analysis window size in samples
  default: 512

draft: false
---

[noisegate~] is a noise gate abstraction. It takes a threshold in dBFS in which it only audio through that has a RMS value over that threshold.
