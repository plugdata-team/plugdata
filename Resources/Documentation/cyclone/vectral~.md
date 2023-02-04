---
title: vectral~

description: smooth/filter frame based signal data

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: vector/block size
  default: 512

inlets:
  1st:
  - type: signal
    description: output index of the processed vector/block
  2nd:
  - type: signal
    description: input index of the vector/block being processed
  3rd:
  - type: signal
    description: the signal vector/block to smooth/filter

outlets:
  1st:
  - type: signal
    description: smoothened/filtered block/vector signal

methods:
  - type: rampsmooth <f, f>
    description: linear ramp mode, values are <up, down> time
    default: 
  - type: slide <f, f>
    description: logarithmic ramp mode, values are <up, down> time
    default:
  - type: deltaclio <f, f>
    description: change limited mode, values are <up, down> limit
    default:
  - type: clear
    description: sets the last frame values to "0"
    default:
  - type: size <float>
    description: sets the vector/block size
    default: 

draft: true
---

[vectral~] is useful for smoothening/filtering frame based signal data such as the output of [fft~], mostly for viewing purposes.
