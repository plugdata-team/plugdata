---
title: stereo.rev~

description: stereo reverb

categories:
- object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: decay in seconds 
  default: 1
- type: float
  description: room size (0-1) 
  default: 0.5
- type: float
  description: high frequency damping
  default: 0
- type: float
  description: wet ratio
  default: 0.5

inlets:
  1st:
  - type: signal
    description: left input signal
  2nd:
  - type: signal
    description: right input signal

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output


methods:
  - type: decay <float>
    description: decay time in seconds
  - type: damp <float>
    description: high frequency damping in Hz
  - type: size <float>
    description: room size (0-1)
  - type: wet <float>
    description: room size (0-1)
  - type: clear
    description: clears delay buffers

draft: false
---

[mono.rev~] is a stereo input/output reverb abstraction, a variant of [mono.rev~] with two independent reverb channels.
