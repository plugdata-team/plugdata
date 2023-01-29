---
title: freeze~

description: freeze

categories:
- object

pdcategory:

arguments:

inlets:
  1st:
  - type: signal
    description: input to freeze
  2nd:
  - type: float
    description: non-zero freezes (or refreezes), 0 unfreezes

outlets:
  1st:
  - type: signal
    description: freeze output

methods:
  - type: freeze
    description: (re)freezes
  - type: unfreeze
    description: unfreezes

draft: false
---

[freeze~] is an abstraction based on [sigmund~] (analysis & ressynthesis). It contains a bank with oscillators.

