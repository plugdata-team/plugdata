---
title: autotune

description: Retune to a close scale step

categories:
- object

pdcategory: Audio Filters, General Audio Manipulation

arguments:
  - description: scale in cents
    type: list
    default: equal temperament

inlets:
  1st:
  - type: float
    description: MIDI pitch to be retuned
  - type: base <float>
    description: MIDI pitch base
  - type: bypass <float>
    description: non-zero sets to bypass mode
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: float
    description: retuned MIDI pitch

draft: false
---

[autotune] receives a scale as a list of steps in cents and a base MIDI pitch value (default 60). It then retunes incoming MIDI pitches to the closest step in the scale.
