---
title: fader~
description: fader waveshaper

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: symbol
  description: fade types <quartic, sin, sqrt, hann, lin, hannsin, linsin>
  default: quartic

inlets:
  1st:
  - type: float/signal
    description: input value (clipped from 0 to 1)

outlets:
  1st:
  - type: signal
    description: waveshaped output

methods:
  - type: anything
    description: fade types <quartic, sin, sqrt, hann, lin, hannsin, linsin>

draft: false
---

[fader~] is a waveshaper. It takes input from 0 to 1 and uses internal lookup tables for 7 different fading curves.

