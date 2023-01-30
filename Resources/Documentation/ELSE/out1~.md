---
title: out1~

description: mono output

categories:
- object

pdcategory: ELSE, Audio I/O

arguments:
  - description: max gain (needs to be > 0)
    type: float
    default: 1
  - description: scaling mode: 0 (quartic), 1 (dB), or 2 (linear)
    type: float
    default: 0

inlets:
  1st:
  - type: signal
    description: incoming signal routed to a mono output output: [dac~ 1]
  - type: float
    description: sets slider (range is clipped to 0-1)

outlets:

flags:
  - name: -ch <float>
    description: sets output channel (default 1)

methods:
  - type: <on/off>
    description: turns DSP Engine (Compute Audio) On or Off
  - type: mode <float>
    description: scaling mode: 0 (quartic, default), 1 (dB) or 2 (linear)
  - type: mute
    description: mute/unmute button
  - type: gain <float>
    description: sets max gain (values <= 0 are ignored)
  - type: ramp <float>
    description: sets ramp time in ms for slider values (default 20)

draft: false
---

[out1~] is variant of [out~] and a convenient mono output abstraction. It has controls for mute, DSP on/off, ramp time, maximum gain and scaling mode.
