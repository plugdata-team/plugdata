---
title: chorus~

description: Chorus effect

categories:
- object

pdcategory: DSP (assorted)

arguments:
  1st:
  - description: rate in hertz
    type: float
    default: 0
  2nd:
  - description: depth
    type: float
    default: 0 
  3rd:
  - description: dry (0) to wet (1)
    type: float
    default: 0

inlets:
  1st:
  - type: signal
    description: input to chorus
  2nd:
  - type: float
    description: rate in hertz
  3rd:
  - type: float
    description: depth
  4th:
  - type: float
    description: dry/wet (0-1)

outlets:
  1st:
  - type: signal
    description: chorus output

draft: false
---

[chorus~] is a simple mono chorus effect abstraction. It uses an internal comb filter and has only one extra voice.