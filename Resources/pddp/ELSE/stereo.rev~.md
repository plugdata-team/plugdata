---
title: stereo.rev~

description:

categories:
- object

pdcategory:

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
  - type: decay <f>
    description: decay time in seconds
  - type: damp <f>
    description: high frequency damping in hertz
  - type: size <f>
    description: room size (0-1)
  - type: wet <f>
    description: room size (0-1)
  - type: clear
    description: clears delay buffers
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

draft: false
---

[mono.rev~] is a stereo input/output reverb abstraction, a variant of [mono.rev~] with two independent reverb channels.
