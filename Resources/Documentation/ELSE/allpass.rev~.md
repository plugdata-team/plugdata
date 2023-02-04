---
title: allpass.rev~

description: allpass reverberator

categories:
 - object

pdcategory: ELSE, Filters, Effects

arguments:
  - type: float
    description: maximum and initial delay time in ms
    default: 0
  - type: float
    description: decay time in ms
    default: 0
  - type: float
    description: non-0 sets to gain mode
    default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: delay time in ms
  3rd:
  - type: float/signal
    description: decay time in ms

outlets:
  1st:
  - type: signal
    description: the filtered signal

methods:
  - type: size <float>
    description: changes the max delay size (in ms)
  - type: clear
    description: clears the delay buffer
  - type: gain <float>
    description: non-0 sets to gain mode

draft: false
---

[allpass.rev~] serves as a reverberator, allpass filter and delay. By default, you can set a delay time and a reverberation/decay time in ms ("t60"), which is the time the impulse takes to fall 60dB in energy (but you can change this parameter to a gain coefficient).
