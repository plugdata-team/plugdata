---
title: resonbank2~

description: bank of resonators

categories:
- object

pdcategory: ELSE, Filters, Effects

inlets:
  1st:
  - type: float/signal
    description: fundamental frequency in Hz
  2nd:
  - type: signal
    description: excitation signal
  3rd:
  - type: float/signal
    description: time multiplier
outlets:
  1st:
  - type: signals
    description: real part of the resonator bank
  2nd:
  - type: signals
    description: imaginary part of resonator bank

flags:
  - name: -partial <list>
    description: list of frequencies for all filters
  - name: -decay <list>
    description: list of decay times for all filters
  - name: -amp <list>
    description: list of amplitudes for all filters
  - name: -ramp <list>
    description: list of ramp times for all filters
  - name: -rampall <float>
    description: sets ramp time for all filters
  - name: -mc
    description: sets to multichannel output

methods:
  - type: partial <list>
    description: sets list of partials for the resonators's frequencies
  - type: decay <list>
    description: list of decay times for all filters
  - type: amp <list>
    description: list of amplitudes for all filters
  - type: ramp <list>
    description: list of ramp times for all filters
  - type: rampall <float>
    description: sets ramp time for all filters
  - type: mc <float>
    description: sets to multichannel output
  - type: lop
    description: set to lowpass mode
  - type: bp
    description: set to bandpass mode
  - type: hip
    description: set to highpass mode

draft: false
---

[resonbank2~] is a bank of resonators made of [resonator2~] objects. The design and structure here is different than [resonator2~] in order to make it better suited for sound synthesis. It has the same design as [resonbank~] and has a list of partials for each resonator and a frequency multiplier in the left inlet, kinda like [oscbank~]. The mid inlet the trigger (or excitation) signal (kinda like [pluck~]) - this must be an impulse since noise sources and noise bursts generate very loud outputs! The rightmost inlet is a time multiplier for the decay times. The number of filters is set via the parameter list size (such as the partial list). There's also support for multichannel output.
