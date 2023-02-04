---
title: vocoder~

description: channel Vocoder

categories:
- object

pdcategory: ELSE, Effects, Filters

arguments:
- type: float
  description: number of channels (obligatory)
  default:
- type: float
  description: filter Q for all channels
  default: 50
- type: list
  description: List of frequency (in MIDI) for each channel
  default: equally dividing the range in MIDI from 28 and 108 for the number of channels

inlets:
  1st:
  - type: signal
    description: synth source input
  - type: list
    description: list of frequencies (in MIDI) for each channel
  2nd:
  - type: signal
    description: control source input
  3rd:
  - type: float
    description: filter Q for all channels

outlets:
  1st:
  - type: signal
    description: vocoder output

draft: false
---

[vocoder~] is classic cross synthesis channel vocoder abstraction.
