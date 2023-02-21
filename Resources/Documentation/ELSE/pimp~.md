---
title: pimp~
description: phasor~ + imp~

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default:
- type: float
  description: initial phase offset
  default:

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: phase signal
  2nd:
  - type: signal
    description: impulse signal (at period transitions)

draft: false
---

[pimp~] is a combination of [phasor~] and [imp~] (alias of [impulse~]). It's [imp~] with an extra phase output (left outlet). The impulse happens at every phase period start (in both negative or positive frequencies/directions). It also has inlets for phase sync and phase modulation like [imp~].

