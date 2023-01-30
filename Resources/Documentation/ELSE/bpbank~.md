---
title: bpbank~

description: bank of bandpass filters

categories:
- object

pdcategory: ELSE, Filters

arguments:
  - description: number of bandpass filters
    type: float
    default: 1
  - description: ramp time in ms
    type: float
    default: 10

inlets:
  1st:
  - type: signal
    description: signal to be filtered via a bank of bandpass filters

outlets:
  1st:
  - type: signal
    description: the signal filtered by the bank of filters

flags:
- name: -freq <list>
  description: sets list of frequencies for all filters
- name: -amp <list>
  description: sets list of amplitudes for all filters
- name: -q <list>
  description: sets list of resonance (Q) for all filters
- name: -ramp <list>
  description: sets list of ramp time for all filters
- name: -rampall <float>
  description: sets ramp time for all filters

methods:
  - type: freq <list>
    description: list of frequencies for all filters in the bank
  - type: q <list>
    description: list of resonance (Q) for all filters in the bank
  - type: amp <list>
    description: list of amplitudes for all filters in the bank
  - type: ramp <list>
    description: list of ramp time for all filters in the bank
  - type: rampal <float>
    description: ramp time for all filters in the bank

draft: false
---

[bpbank~] is a bank made of [bandpass~] objects. You can set any number of filters with the first argument and control the parameters for each filter. If you use flags, the number of elements in the list (such as the frequency list) sets the number of filters in the bank (you shouldn't use arguments if you use flags).
