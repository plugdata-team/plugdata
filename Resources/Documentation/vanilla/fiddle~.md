---
title: fiddle~
description: pitch estimator and sinusoidal peak finder
pdcategory: vanilla, Analysis
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed
  - type: bang
    description: poll current value (if auto-mode is off)
outlets:
  1st:
  - type: float
    description: cooked pitch output
  2nd:
  - type: bang
    description: a bang is sent on new attacks
  3rd:
  - type: list
    description: continuous raw pitch and amplitude
  4th:
  - type: float
    description: amplitude in dB
  5th:
  - type: list
    description: peak number, frequency and amplitude

draft: false

arguments:
- description: window size
  default: 1024
  type: float
- description: number of pitch outlets
  default: 1
  type: float
- description: number of peaks to find
  default: 20
  type: float
- description: number of peaks to output
  default: 0
  type: float

methods:
- type: amp-range <float, float>
  description: set low and high amplitude threshold
- type: vibrato <float, float>
  description: set period in msec and pitch deviation for "cooked" pitch outlet
- type: reattack <float, float>
  description: set period in msec and amplitude in dB to report re-attack
- type: auto <float>
  description: nonzero sets to auto mode (default) and zero sets it off
- type: npartial <float>
  description: set partial number to be weighted half as strongly as the fundamental
- type: npoints <float>
  description: sets number of points in analysis window
- type: print <float>
  description: print out settings

