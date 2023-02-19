---
title: giga.rev~
description: Giga reverb

categories:
 - object

pdcategory: ELSE, Effects

arguments:

inlets:
  1st:
  - type: signal
    description: signal input (mono in, stereo out)

outlets:
  1st:
  - type: signal
    description: left output channel of the reverb
  2nd:
  - type: signal
    description: right output channel of the reverb

flags:
  - name: -maxsize <float>
    description: maximum room size in square meters (default 300)
  - name: -size <float>
    description: room size in square meters (default 50)
  - name: -decay <float>
    description: decay time in seconds (default 7)
  - name: -damp <float>
    description: high frequency damping (default 0.5)
  - name: -spread <float>
    description: stereo spread (default 0.5)
  - name: -bw <float>
    description: input bandwidth (default 0.5)
  - name: -dry <float>
    description: dry level (default 0.5)
  - name: -early <float>
    description: early reflections level (default 0.25)
  - name: -late <float>
    description: late reflections level (default 0.25)
  - name: -wet <float>
    description: wet level (default 1)

methods:
  - type: size <float>
    description: room size in square meters
  - type: damp <float>
    description: high frequency damping (0-1)
  - type: decay <float>
    description: decay time in seconds (0.001-3600)
  - type: bw <float>
    description: input bandwidth (0-1)
  - type: spread <float>
    description: stereo spreading (0-1)
  - type: early <float>
    description: early reflections gain level (0-1)
  - type: late <float>
    description: late reflections gain level (0-1)
  - type: dry <float>
    description: dry gain level (0-1)
  - type: wet <float>
    description: wet gain level (0-1) - overrides early and late
  - type: clear
    description: clears delay lines
  - type: print
    description: print parameters on Pd Window

draft: false
---

[giga.rev~] is based on the well known "Gigaverb" algorithm by Juhana Sadeharju.

