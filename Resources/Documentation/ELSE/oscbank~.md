---
title: oscbank~

description: bank of oscillators

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: number of oscillators 
    type: float
    default: 1
  - description: fundamental frequency in Hz
    type: float
    default: 0
  - description: ramp time in ms
    type: float
    default: 10

inlets:
  1st:
  - type: float/signal
    description: fundamental frequency in Hz

outlets:
  1st:
  - type: signal
    description: the oscillator bank output

flags:
  - name: -ratio <list>
    description: sets list of ratios for all oscillators (default all 1)
  - name: -amp <list>
    description: sets list of amplitudes for all oscillators (default all 1)
  - name: -phase <list>
    description: sets list of phases for all oscillators (default all 0)
  - name: -ramp <list>
    description: sets list of ramp times for all oscillators
  - name: -freq <float>
    description: sets fundamental frequency in Hz (default 0)
  - name: -rampall <float>
    description: sets a ramp time for all oscillators (default all 10)

methods:
  - type: ratio <list>
    description: list of ratios for all oscillators
  - type: amp <list>
    description: list of amplitudes for all oscillators
  - type: phase <list>
    description: list of phases (0-1) for all oscillators
  - type: ramp <list>
    description: list of ramp time for all oscillators
  - type: rampall <float>
    description: sets ramp time for all oscillators

draft: false
---

[oscbank~] is a bank made of [sine~] objects. You can set any number of oscillators and control their parameters. Unlike [oscbank2~], you have a fundamental frequency input and the frequency of each oscillator is specified as a ratio of that frequency. If you use flags, the number of elements in the list (such as amplitude list) sets the number of oscillators in the bank, and you must not use regular arguments in this case.
