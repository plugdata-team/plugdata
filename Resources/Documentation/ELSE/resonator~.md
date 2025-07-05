---
title: resonator~

description: resonant complex one pole filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: frequency
  default: 0
- type: float
  description: t60 decay time in ms
  default: 0

methods:
  - type: clear
    description: clears filter's memory
  - type: lop
    description: set to lowpass mode
  - type: bp
    description: set to bandpass mode
  - type: hip
    description: set to highpass mode

inlets:
  1st:
  - type: float/signal
    description: central frequency in Hz
  2nd:
  - type: signal
    description: excitation signal
  3rd:
  - type: float/signal
    description: t60 decay time in ms (resonance)

outlets:
  1st:
  - type: signal
    description: resonator/filtered signal

flags:
- name: -lop
  description: set lowpass mode
- name: -hip
  description: set highpass mode

draft: false
---
[resonator~] is just like [resonant~], but the resonance parameter is defined as resonance time in 't60'. It has 3 modes, the default is bandpass, which is like [resonant~], then you can also have lowpass (like [lowpass~]) and highpass (you guessed it, like [highpass~]). Note that a t60 value of 0 bypasses the input! For a bank of [resonator~] objects, see [resonbank~]. For another similar filter, see [resonator2~].
