---
title: synth~

description: mono/poly synth abstraction loader

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: symbol
  description: abstraction name
  default: template
- type: float
  description: (optional) number of voices
  default: 1
- type: list
  description: (optional) extra arguments sent to the abstraction
  default: 0

inlets:
  1st:
  - description: midi note messages, pitch and velocity
    type: list
  - description: custom messages to synth abstraction
    type: anything

outlets:
  1st:
  - description: synth signal output
    type: signal

flags:
  - name: -v <float>
    description: sets number of voices (default 1/mono)
  - name: -mc
    description: sets to multichannel output (default single channel)

methods:
  - type: load <symbol, float>
    description: abstraction name and optional number of voices
  - type: voices <float>
    description: sets number of voices (1 sets to monophonic)
  - type: mc <float>
    description: non zero sets to multichannel output

draft: false
---

The [synth~] object loads synth abstractions that follow a simple template. It is just a wrapper over [clone] and [mono]/[voices] but may provide some convenience, specially to newbies. One convenience is the ability to open/load different abstractions in runtime. The arguments are abstraction name and number of voices. If no arguments are given, a default and simple abstraction is loaded as a template and basis to build new synths. Clicking on the object opens the loaded abstration. Shift click opens a panel to choose an abstraction.