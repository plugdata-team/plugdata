---
title: vocoder~

description: Channel Vocoder

categories:
- object

pdcategory: Audio Filter

arguments:
- description: number of channels (obligatory)
  type: float
  default:
- description: filter Q for all channels
  type: float
  default: 50
- description: List of frequency (in MIDI) for each channel
  type: list
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