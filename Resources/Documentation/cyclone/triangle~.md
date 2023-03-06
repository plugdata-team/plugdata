---
title: triangle~

description: variable duty triangular wave

categories:
 - object

pdcategory: cyclone, Signal Generators

arguments:
- type: float
  description: peak position from 1 to 0
  default: 0.5

inlets:
  1st:
  - type: signal
    description: phase input signal
  2nd:
  - type: signal
    description: variable duty (peak position) from 1 to 0

outlets:
  1st:
  - type: signal
    description: the triangle wavetable output based on args
  
flags:
  - name: lo <float>
    description: lowest point
    default: 1
  - name: hi <float>
    description: highest point
    default: 1

methods:
  - type: lo <float>
    description: sets lowest point
    default: 1
  - type: hi <float>
    description: sets highest point
    default: 1

draft: true
---

[triangle~] is a triangular wavetable that is read with phase values from 0 to 1 into the first inlet- a [phasor~] input turns it into a wavetable oscillator.
A second inlet changes the position of the peak value (variable duty, from 0 to 1), going from sawtooth like waveforms to triangular ones.
The default lo/hi points are -1 and 1, but may be changed using the lo/hi messages to any given range.
