---
title: free.rev~

description: free reverb

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: decay or 'liveness' (0-1)
  type: float
  default: 0.5
- description: high frequency damping (0-1)
  type: float
  default: 0
- description: stereo width (0-1)
  type: float
  default: 0.5
- description: wet ratio (0-1)
  type: float
  default: 1

inlets:
  1st:
  - type: signal
    description: left channel input signal
  2nd:
  - type: signal
    description: right channel input signal

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output

methods:
  - type: decay <float>
    description: decay or 'liveness' (0-1)
  - type: damp <float>
    description: high frequency damping (0-1)
  - type: width <float>
    description: stereo width (0-1)
  - type: wet <float>
    description: wet ratio (0-1)
  - type: clear
    description: clears delay lines


draft: false
---

[free.rev~] is a stereo reverb abstraction based on the widely known 'freeverb' algorithm.

