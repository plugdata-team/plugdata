---
title: autotune

description: retune to a close scale step

categories:
- object

pdcategory: ELSE, Tuning

arguments:
  - description: scale in cents
    type: list
    default: equal temperament

flags:
- name: -base <float> 
  description: base MIDI pitch (default 60)

inlets:
  1st:
  - type: float
    description: MIDI pitch to be retuned
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: float
    description: retuned MIDI pitch

methods:
  - type: base <float>
    description: MIDI pitch base
  - type: bypass <float>
    description: non-0 sets to bypass mode

draft: false
---

[autotune] receives a scale as a list of steps in cents and a base MIDI pitch value (default 60). It then retunes incoming MIDI pitches to the closest step in the scale.
