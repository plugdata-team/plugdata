---
title: range

description: Anylize range

categories:
- object

pdcategory: General

arguments:(none)

inlets:
  1st:
  - type: float/signal
    description: input signal to analize

outlets:
  1st:
  - type: signal
    description: minimum range
  2nd:
  - type: signal
    description: maximum range

methods:
  - type: reset
    description: resets object

draft: false
---

[range~] analyxes a float input's range values.
