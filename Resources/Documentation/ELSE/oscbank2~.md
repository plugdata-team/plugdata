---
title: oscbank2~

description: bank of oscillators

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: number of oscillators 
    type: float
    default: 1
  - description: ramp time in ms
    type: float
    default: 10

inlets:
  1st:
  - type: freq <list>
    description: list of frequencies for all oscillators in the bank
  - type: amp <list>
    description: list of amplitudes for all oscillators in the bank
  - type: phase <list>
    description: list of phases (0-1) for all oscillators in the bank
  - type: ramp <list>
    description: list of ramp time for all oscillators in the bank
  - type: rampall <float>
    description: sets ramp time for all oscillators

outlets:
  1st:
  - type: signal
    description: the oscillator bank output

flags:
  - name: -freq <list>
    description: sets list of frequencies for all oscillators
  - name: -amp <list>
    description: sets list of amplitudes for all oscillators
  - name: -phase <list>
    description: sets list of phases for all oscillators
  - name: -ramp <list>
    description: sets ramp time for all oscillators
  - name: -rampall <float>
    description: sets a ramp time for all oscillators (default all 10)

methods:
  - type: freq <list>
    description: list of frequencies for all oscillators in the bank
  - type: amp <list>
    description: list of amplitudes for all oscillators in the bank
  - type: phase <list>
    description: list of phases (0-1) for all oscillators in the bank
  - type: ramp <list>
    description: list of ramp time for all oscillators in the bank
  - type: rampall <float>
    description: sets ramp time for all oscillators

draft: false
---

[oscbank2~] is a bank made of [sine~] objects. You can set any number of oscillators and control their parameters. If you use flags, the number of elements in the list (such as amplitude list) sets the number of oscillators in the bank, and you must not use regular arguments in this case.
