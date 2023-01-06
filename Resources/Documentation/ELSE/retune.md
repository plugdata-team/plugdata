---
title: retune

description: Retune to a given scale

categories:
- object

pdcategory:

arguments:
- type: list
  description: scale in cents
  default: equal temperament

inlets:
  1st:
  - type: float
    description: MIDI pitch to be retuned
  - type: base <float>
    description: MIDI pitch base (decimals allowed)
  2nd:
  - type: list
    description: scale in cents

outlets:
  1st:
  - type: list
    description: scale in cents

flags:
- name: -base <f>
  description: base MIDI pitch (default 60)

draft: false
---

[retune] receives a scale as a list of steps in cents and a base MIDI pitch value (default 60). It then remaps the incoming MIDI ptches (as integers) and retunes the pitches above or below the base value according to the scale.
