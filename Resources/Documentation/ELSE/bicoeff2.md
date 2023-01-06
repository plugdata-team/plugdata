---
title: bicoeff2

description: Biquad coefficient generator

categories:
 - object

pdcategory: Audio Math

arguments:
  1st:
  - type: symbol
    description: (optional) sets type <hipass/etc>, (default='off')
  2nd:
  - type: float
    description: sets cutoff/center frequency (default=0)
  3rd:
  - type: float
    description: sets Q/slope (default=1)
  4th:
  - type: float
    description: sets gain in db (default=0)

inlets:
  1st:
  - type: float
    description: sets cutoff/center frequency
  - type: bang
    description: generates coefficients
  - type: list <f, f, f>
    description: sets frequency, Q/Slope and gain, and then outputs coefficients
  - type: qs <float>
    description: sets "Q" or "Slope" and outputs coefficients
  - type: gain <float>
    description: sets gain in db and outputs coefficients
  - type: anything
    description: sets filter type <allpass, lowpass, highpass, bandpass, resonant, bandstop, eq, lowshelf, highshelf, off>. It takes 3 more optional arguments that set frequency, Q/Slope and gain
  2nd:
  - type: float
    description: sets "Q" or "Slope" and outputs coefficients
  3rd:
  - type: float
    description: sets gain in db

outlets:
  1st:
  - type: list
    description: 5 coefficients for the vanilla [biquad~] object

draft: false
---

[bicoeff2] generates coefficients for vanilla's [biquad~] according to different filter types. All inlets are hot!