---
title: trapezoid~

description: trapezoidal wavetable

categories:
 - object

pdcategory: cyclone, Signal Generators

arguments:
- type: float
  description: ramp up
  default: 0.1
- type: float
  description: ramp down
  default: 0.1

inlets:
  1st:
  - type: signal
    description: input phase signal
  2nd:
  - type: signal
    description: ramp up
  3rd:
  - type: signal
    description: ramp down

outlets:
  1st:
  - type: signal
    description: trapezoidal waveform output

flags:
  - name: @lo <float>
    description: lowest point
    default: 0
  - name: @hi <float>
    description: highest point
    default: 1

methods:
  - type: lo <float>
    description: sets lowest point
    default: 0
  - type: hi <float>
    description: sets highest point
    default: 1

draft: true
---

[trapezoid~] is a trapezoidal wavetable that is read with phase values from 0 to 1 into the first inlet- a [phasor~] input turns it into a wavetable oscillator. A second and third inlet change the position of ramp up/down points.
The default lo/hi values are 0 and 1, but may be changed using the lo/hi messages or attributes to any given range.
