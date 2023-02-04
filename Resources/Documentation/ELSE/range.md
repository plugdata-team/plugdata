---
title: range

description: analyze range

categories:
- object

pdcategory: ELSE, Analysis

arguments:

inlets:
  1st:
  - type: float/signal
    description: input signal to analyze

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
