---
title: plate.rev~

description: plate reverb

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: pre delay in ms (0-1000)
  type: float
  default: 50
- description: filter cutoff (0-1)
  type: float
  default: 0.5
- description: high frequency damping (0-1)
  type: float
  default: 0.25
- description: room size (0-1)
  type: float
  default: 0.75
- description: wet ratio (0-1)
  type: float
  default: 0.75

inlets:
  1st:
  - type: signal
    description: input signal

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output 

methods:
  - type: pre <float>
    description: pre delay in ms (0-1000)
  - type: cutoff <float>
    description: filter cutoff (0-1)
  - type: damp <float>
    description: high frequency damping (0-1)
  - type: size <float>
    description: room size feedback (0-1)
  - type: wet <float>
    description: wet ratio (0-1)
  - type: clear
    description: clears delay buffers

draft: false
---

[plate.rev~] is a reverb abstraction based on a patch by Tom Erbe implementing the plate reverb model by Dattorro. It has a mono input and stereo output.

