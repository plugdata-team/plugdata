---
title: blip~

description: bandlimited cosine oscillator  

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: fundamental frequency in Hz
  default: 0
- type: float
  description: number of partials
  default: 1
- type: float
  description: spectral multiplier
  default: 1
- type: float
  description: lowest harmonic
  default: 1

flags:
  - name: -midi
    description: sets frequency input in MIDI pitch (default Hz)
  - name: -soft
    description: sets to soft sync mode (default hard)

inlets:
  1st:
  - type: float/signal
    description: fundamental frequency in Hz
  2nd:
  - type: signal
    description: impulses reset phase
  3rd:
  - type: float/signal
    description: phase modulation input
  4th:
    - type: float/signal
      description: spectral multiplier

outlets:
  1st:
  - type: signal
    description: band limited oscillator output

methods:
  - type: n <float>
    description: number of partials
  - type: low <float>
    description: lowest harmonic
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode

draft: false
---

[blip~] uses DSF (Discrete-Summation Formulae) to generate waveforms as a sum of cosines. It takes a frequency in Hz for the fundamental, a number of harmonics, a multiplier for the partials after the first one and the lowest harmonic number (by default it generates an impulse waveform). This object is based on Csound's 'gbuzz' opcode.
