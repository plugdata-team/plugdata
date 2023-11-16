---
title: filterdelay~

description: filtered delay

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: delay time in ms
  default: 0
- type: float
  description: feedback amount
  default: 0

inlets:
  1st:
  - description: input signal
    type: signal
  2nd:
  - description: delay time in ms
    type: signal
  3rd:
  - description: feedback amount
    type: signal

outlets:
  1st:
  - description: filter delay output
    type: signal

flags:
  - name: -cutoff <float>
    description: sets cutoff
  - name: -reson <float>
    description: sets resonance
  - name: -wet <float>
    description: sets dry/wet mix

methods:
  - name: clear
    description: clears delay line
  - name: -cutoff <float>
    description: sets cutoff
  - name: -reson <float>
    description: sets resonance
  - name: -wet <float>
    description: sets dry/wet mix

draft: false
---

[filterdelay~] is a high level delay unit that goes thgouh a resonant lowpass filter, a soft clipper and a DC filter. The processed delay output is also fed back into the delay line.