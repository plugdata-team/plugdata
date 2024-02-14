---
title: pan~

description: circular panning over given number of channels

categories:
- object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of output channels (min 2, max 4096)
  default: 2
- type: float
  description: spread
  default: 1
- type: float
  description: offset in degrees
  default: 0

methods:
  - type: offset <float>
    description: set offset angle in degrees
  - type: radians <float>
    description: set offset angle in radians

inlets:
  1st:
  - type: signal
    description: signal input
  2nd:
  - type: float/signal
    description: gain parameter
  3rd:
  - type: float/signal
    description: azimuth  (normalized from 0-1 or radians)
  4th:
  - type: float/signal
    description: spread parameter (from -1 to 1)

outlets:
  nth:
  - type: signal
    description: equal power panned output $nth

flags:
  - name: -radians
    description: change 3rd inlet input from degrees to radians

draft: false
---

[pan~] pans an input signal to 'n' specified outlets with equal power crossfading between adjacent channels according to a spread parameter. The speakers are supposedly disposed in a circular setting with equidistant angles. The output selection can then be considered an azimuth angle input whose range is normalized from 0 to 1, where 1 goes back to 0!
