---
title: fdn.rev~
description: feedback delay network reverberator

categories:
 - object

pdcategory: ELSE, Effects

arguments:
inlets:
  1st:
  - type: signal
    description: input signal to reverberate
  - type: list
    description: sets reflection times in ms
  2nd:
  - type: float
    description: decay time in seconds (t60)
  3rd:
  - type: float
    description: high frequency damping in % (from 0 to 100)

outlets:
  1st:
  - type: signal
    description: left output of the FDN reverberator
  2nd:
  - type: signal
    description: right output of the FDN reverberator

flags:
  - name: -time <float>
    description: t60 reverberation time in seconds (default 4)
  - name: -damping <float>
    description: high frequency damping in % (default 0)

methods:
  - type: time <float>
    description: reverberation decay time in seconds (t60)
  - type: damping <float>
    description: high frequency damping in % (from 0 to 100)
  - type: set <list>
    description: set number of lines and min/max times
  - type: exp <float>
    description: non-0 sets delay times exponentially
  - type: clear
    description: clears the delay lines and reverberation
  - type: print
    description: print delay lines and other parameters in Pd's window

draft: false
---

[fdn.rev~] is a feedback delay network reverberator which can be used for late reflections (a.k.a reverb tail). The main parameters are: decay time (t60) and high frequency damping.

