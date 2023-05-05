---
title: fbdelay~
description: feedback delay line

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: delay time
  default: 1 sample
- type: float
  description: feedback amount
  default: 0
- type: float
  description: feedback mode: <0> ("t60") or <non-0> (gain mode)
  default: 0

inlets:
  1st:
  - type: signal
    description: signal input to the delay line
  2nd:
  - type: float/signal
    description: delay time (in ms)
  3rd:
  - type: float/signal
    description: decay time (in ms)

outlets:
  1st:
  - type: signal
    description: the filtered/delayed signal

flags:
  - name: -size <float>
    description: sets delay size (default 1000 ms or first argument's value if given)
  - name: -samps
    description: sets delay time unit to "samples" (default is ms)
  - name: -gain
    description: sets feedback mode to gain

methods:
  - type: size <float>
    description: changes the maximum delay size (in ms)
  - type: freeze <float>
    description: non-0 freezes, zero unfreezes
  - type: clear
    description: clears the delay buffer
  - type: gain <float>
    description: non-0 sets to gain mode

draft: false
---

Use [fbdelay~] for delay effects, reverberation and comb filtering. By default, you can set a delay time and a reverberation/decay time in ms ("t60"), which is the time the impulse takes to fall 60dB in energy (but you can change this parameter to a gain coefficient).

