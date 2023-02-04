---
title: resonbank~

description: bank of resonant filters

categories:
- object

pdcategory: ELSE, Filters, Effects

arguments:
- type: float
  description: number of filters
  default: 1
- type: float
  description: ramp time in ms
  default: 10

inlets:
  1st:
  - type: signal
    description: signal to be filtered via a bank of resonators

outlets:
  1st:
  - type: signal
    description: filtered signal

flags:
  - name: -freq <list>
    description: list of frequencies for all filters
  - name: -attack <list>
    description: list of attack times for all filters
  - name: -decay <list>
    description: list of decay times for all filters
  - name: -amp <list>
    description: list of amplitudes for all filters
  - name: -ramp <list>
    description: list of ramp times for all filters
  - name: -rampall <float>
    description: sets ramp time for all filters


methods: 
  - type: freq <list>
    description: list of frequencies for all filters
  - type: attack <list>
    description: list of attack times for all filters
  - type: decay <list>
    description: list of decay times for all filters
  - type: amp <list>
    description: list of amplitudes for all filters
  - type: ramp <list>
    description: list of ramp times for all filters
  - type: rampall <float>
    description: sets ramp time for all filters

draft: false
---

[resonbank~] is a bank made of [resonant~] objects. You can set any number of filters and control their parameters. If you use flags, the number of elements in the list (such as the frequency list) sets the number of filters in the bank (you shouldn't use arguments if you use flags).
