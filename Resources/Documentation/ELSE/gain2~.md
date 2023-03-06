---
title: gain2~

description: stereo gain

categories:
- object

pdcategory: ELSE, Effects, Mixing and Routing

arguments:

inlets:
  1st:
  - type: signal
    description: incoming signal whose gain will be adjusted
  - type: bang
    description: mute/unmute
  2nd:
  - type: signal
    description: right channel signal whose gain will be adjusted

outlets:
  1st:
  - type: signal
    description: the adjusted gain signal of the left channel
  2nd:
  - type: signal
    description: the adjusted gain signal of the right channel
  3rd:
  - type: float
    description: slider value, from 0-1 (independent of maximum gain)

flags:
  - name: -max <float>
    description: sets max gain (needs to be > 0, default 1)
  - name: -mode <float>
    description: sets scaling mode: 0 (quartic, default), 1 (dB) or 2 (linear)
  - name: -ramp <float>
    description: sets ramp time (minimum 5, default 20)
  - name: -init
    description: sets to init mode (default no init)

methods:
  - type: set <float>
    description: sets slider (range is clipped to 0-1)
  - type: mode <float>
    description: scaling mode: 0 (quartic, default), 1 (dB) or 2 (linear)
  - type: gain <float>
    description: sets max gain (values <= 0 are ignored)
  - type: ramp <float>
    description: sets ramp time in ms for slider values (default 20)
  - type: init <float>
    description: non-0 sets to init mode

draft: false
---

[gain2~] is a convenient stereo gain abstraction, so you can adjust a stereo signal's gain. It has controls for ramp time, maximum gain, scaling mode and init.

