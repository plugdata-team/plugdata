---
title: cyclone/pink~

description: Pink noise generator

categories:
 - object

pdcategory: cyclone, Random and Noise, Signal Generators

arguments: 
  - type: float
    description: sets seed 
    default: unique internal
  - type: float
    description: pinkness
    default: number of extra octaves

inlets:
  1st:
  - type: signal
    description: set number of octaves (1-31)
  2nd:
  - type: signal
    description: sets pinkness (number of octaves)

outlets:
  1st:
  - type: signal
    description: pink noise

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[pink~] generates pink noise. This is not an actual MAX clone but an object that is borrowed from ELSE which has more functionalities and is backwards compatible to MAX's object since the original just outputs pink noise! White noise has constant spectral power, but pink noise has constant power per octave and it decrease 3dB per octave.