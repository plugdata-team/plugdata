---
title: pvoc.freeze~

description: phase-vocoder freeze

categories:
- object

pdcategory: Effects

arguments:
- description:
  type:
  default:

inlets:
  1st:
  - type: signal
    description: input to freeze
  2nd:
  - type: float
    description: non-zero (re)freezes, 0 unfreezes

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

