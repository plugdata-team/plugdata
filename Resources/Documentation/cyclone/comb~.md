---
title: comb~
description: comb filter
categories:
 - object
pdcategory: cyclone, Filters, Effects
arguments:
- type: float
  description: maximum delay time in ms
  default: 10
- type: float
  description: D: delay time in ms
  default: 0
- type: float
  description: a: input gain coefficient
  default: 0
- type: float
  description: b: feedforward gain coefficient
  default: 0
- type: float
  description: c: feedback gain coefficient
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to pass through comb filter
  - type: list
    description: updates all 5 arguments
  2nd:
  - type: float/signal
    description: D: delay time in ms
  3rd:
  - type: float/signal
    description: a: input gain coefficient
  4th:
  - type: float/signal
    description: b: feedforward gain coefficient
  5th:
  - type: float/signal
    description: c: feedback gain coefficient
outlets:
  1st:
  - type: signal
    description: output from comb filter

methods:
  - type: clear
    description: clears buffer

draft: false
---

[comb~] is a comb filter, use it for filtering and delay effects.

