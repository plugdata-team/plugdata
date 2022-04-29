---
title: loop~
description: phase generator for looping samplers
categories:
- object
see_also:
- tabread4~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.52'
inlets:
  1st:
  - type: float/signal
    description: transposition value.
  2nd:
  - type: float/signal
    description: window size in samples.
outlets:
  1st:
  - type: signal
    description: phase output from 0 to 1
  2nd:
  - type: signal
    description: sampled window size.
draft: false
---
loop~ takes input signals to set transposition and window size, and outputs a phase and a sampled window size. The window size only changes at phase zero crossings and the phase output is adjusted so that changing window size doesn't change the transposition.

You can send "bang" or "set" message to force the phase to zero--you should mute the output before doing so. This may be desirable if you've set a large window size but then want to decrease it without waiting for the next phase crossing.