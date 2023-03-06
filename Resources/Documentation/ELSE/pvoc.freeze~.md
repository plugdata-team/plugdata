---
title: pvoc.freeze~

description: phase-vocoder freeze

categories:
- object

pdcategory: ELSE, Effects

arguments:

inlets:
  1st:
  - type: signal
    description: input to freeze
  2nd:
  - type: float
    description: non-0 (re)freezes, 0 unfreezes

outlets:
  1st:
  - type: signal
    description: freeze output

methods:
  - type: freeze
    description: freezes/refreezes
  - type: unfreeze
    description: unfreezes

draft: false
---

[pvoc.freeze~] is a freeze object based on a phase vocoder.

