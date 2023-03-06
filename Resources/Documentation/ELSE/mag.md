---
title: mag

description: get spectral magnitudes

categories:
- object

pdcategory: ELSE, Data Math, Analysis

arguments:
- type: symbol
  description: "power" sets to power magnitude spectrum
  default:

inlets:
  1st:
  - type: bang 
    description: converts the last received coordinates pair
  - type: float
    description: real part from the Cartesian coordinates
  2nd:
  - type: float
    description: imaginary part from the Cartesian coordinates

outlets:
  1st:
  - type: float
    description: magnitude

draft: false
---

[mag] gets the spectrum magnitudes (amplitudes) from Cartesian coordinates (real / imaginary). This is much like the amplitude output of [car2pol], but you can also get the power magnitudes instead with the 1st argument.
