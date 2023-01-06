---
title: allpass.rev~

description: Allpass reverberator

categories:
 - object

pdcategory: Audio Filters, Audio Delays, General Audio Manipulation

arguments:
  1st:
  - type: float
    description: maximum and initial delay time in ms
    default: 0
  2nd:
  - type: float
    description: decay time in ms
    default: 0
  3rd:
  - type: float
    description: non-0 sets to gain mode
    default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  - type: size <float>
    description: changes the max delay size (in ms)
  - type: clear
    description: clears the delay buffer
  - type: gain <float>
    description: non-0 sets to gain mode
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

draft: false
---

[allpass.rev~] serves as a reverberator, allpass filter and delay. By default, you can set a delay time and a reverberation/decay time in ms ("t60"), which is the time the impulse takes to fall 60dB in energy (but you can change this parameter to a gain coefficient).
