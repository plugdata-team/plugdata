---
title: mono.rev~

description: mono reverb

categories:
- object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: decay in seconds
  default: 1
- type: float
  description: room size
  default: 0.5
- type: float
  description: high frequency damping
  default: 0
- type: float
  description: wet ratio
  default: 0.5

inlets:
  1st:
  - type: signal
    description: input signal

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output

methods:
  - type: decay <float>
    description: decay time in seconds
  - type: damp <float>
    description: high frequency damping
  - type: size <float>
    description: room size
  - type: wet <float>
    description: wet ratio
  - type: clear
    description: clears delay buffers

draft: false
---

[mono.rev~] is a reverb abstraction with mono input but stereo output. It's based on a feedback delay network like vanilla's [rev3~] object, but it offers a "room size" parameter whose typical value is around 0.5, values closer to 0 and 1 may create unusual results. Changing size values may generates unusual artifacts (you can clear the delay buffers when you do it to prevent it, but you may get clicks).
