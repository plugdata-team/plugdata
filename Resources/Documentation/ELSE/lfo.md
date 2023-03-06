---
title: lfo

description: control LFO

categories:
- object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- description: sets frequency in Hz (default 0)
  type: float
  default: 0
- description: sets initial phase from 0 - 1 (default 0)
  type: float
  default: 0
- description: sets refresh rate in ms (default 5)
  type: float
  default: 5

inlets:
  1st:
  - type: float
    description: frequency in Hz up to 100 (negative values accepted)
  - type: anything
    description: sets waveform (sine, tri, saw, vsaw, and square)
  2nd:
  - type: float
    description: phase reset from 0 - 1
  3rd:
  - type: float
    description: refresh rate in ms (minimum is 5 ms)

outlets:
  1st:
  - type: float
    description: LFO's waveform in the range from 0 - 127

flags:
  - name: -sine
    description: sets waveform to sine (default)
  - name: -tri
    description: sets waveform to triangular
  - name: -saw
    description: sets waveform to sawtooth
  - name: -square <float>
    description: sets waveform to square and width (from 0 to 1)
  - name: -vsaw <float>
    description: sets waveform to variable sawtooth and width (from -1 to 1)


draft: false
---

[lfo] is a control rate LFO with some common waveforms. It doesn't need the DSP on to function,

