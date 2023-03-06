---
title: wave~

description: wavetable reader

categories:
 - object

pdcategory: cyclone, Arrays and Tables

arguments:
- type: symbol
  description: array name (required)
  default: 
- type: float
  description: start point in ms
  default: 0
- type: float
  description: end point in ms
  default: length of the array in ms
- type: float
  description: number of output channels
  default: 1

inlets:
  1st:
  - type: signal
    description: phase signal to read the array
  2nd:
  - type: signal
    description: start point in ms
  3rd:
  - type: signal
    description: end point in ms

outlets:
  1st:
  - type: signal
    description: the specified portion of the given wavetable channel

flags:
  - name: @interp <float>
    description: interpolation mode
    default: 1 (linear)
  - name: @interp_bias <float>
    description:
    default: 0
  - name: @interp_tension <float>
    description:
    default: 0

methods:
  - type: set <symbol>
    description: sets array name
    default: 
  - type: interp <float>
    description: sets interpolation mode <0-7>
    default: 0
  - type: @interp_bias <float>
    description: hermite interpolation bias
    default: 0
  - type: @interp_tension <float>
    description: hermite interpolation tension
    default: 0

draft: true
---

With a phase signal input (from 0 and 1), [wave~] reads a given table. It has many interpolation options and can set a start and end point in the array.
