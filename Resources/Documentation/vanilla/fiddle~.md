---
title: fiddle~
description: Pitch estimator and sinusoidal peak finder
pdcategory: Audio
inlets:
  1st:
  - type: signal
    description: Signal input to be analyzed
  - type: bang
    description: Poll current value (if auto-mode is off)
outlets:
  1st:
  - type: float
    description: Cooked pitch output
  2nd:
  - type: bang
    description: A bang is sent on new attacks.
  3rd:
  - type: list
    description: Continuous raw pitch and amplitude.
  4th:
  - type: float
    description: Amplitude in dB
  5th:
  - type: list
    description: Peak number, frequency and amplitude

draft: false

arguments:
- description: Window size
  default: 1024
  type: float
- description: Number of pitch outlets
  default: 1
  type: float
- description: Number of peaks to find
  default: 20
  type: float
- description: Number of peaks to output
  default: 0
  type: float

methods:
- type: amp-range <float, float>
  description: Set low and high amplitude threshold
- type: vibrato <float, float>
  description: Set period in msec and pitch deviation for "cooked" pitch outlet.
- type: reattack <float, float>
  description: Set period in msec and amplitude in dB to report re-attack.
- type: auto <float>
  description: Nonzero sets to auto mode (default) and zero sets it off.
- type: npartial <float>
  description: Set partial number to be weighted half as strongly as the fundamental
- type: npoints <float>
  description: Sets number of points in analysis window.
- type: print <float>
  description: Print out settings.