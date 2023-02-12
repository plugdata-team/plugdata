---
title: polymetro~

description: polyrhythmic metronome

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: tempo in BPM
  type: float
  default: 120
- description: polyrhythmic multiplier ratio
  type: symbol/float
  default: 1/1

inlets:
  1st:
  - type: float
    description: non-0 starts, zero stops
  - type: bang
    description: syncs/resets
  2nd:
  - type: float
    description: set tempo value in BPM
  3rd:
  - type: anything
    description: multiplier as a float or fraction (anything/symbol/float)

outlets:
  1st:
  - type: signal
    description: impulse base rhythm
  2nd:
  - type: signal
    description: impulse for relative rhythm

draft: false
---

[polymetro~] is a polyrhythmic metronome for signal rate that outputs impulses. Polyrhythms are better specified with fractions, but you can set it as floats, which is a tempo multiplier value in the end. The left output is a counter for the base tempo count while the right outlet is the relative count/tempo as specified by the ratio. This is an abstraction based on [pimp~] and [pimpmul~] objects.

