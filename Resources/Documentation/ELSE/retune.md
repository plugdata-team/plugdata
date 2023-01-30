---
title: retune

description: retune to a given scale

categories:
- object

pdcategory: ELSE, Tuning

arguments:
- type: list
  description: scale in cents
  default: equal temperament

inlets:
  1st:
  - type: float
    description: MIDI pitch to retune
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: list
    description: scale in cents

flags:
- name: -base <f>
  description: base MIDI pitch 
  default: 60

methods:
  - type: base <float>
    description: MIDI pitch base (decimals allowed)

draft: false
---

[retune] receives a scale as a list of steps in cents and a base MIDI pitch value (default 60). It then remaps the incoming MIDI pitches (as integers) and retunes the pitches above or below the base value according to the scale.
