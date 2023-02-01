---
title: hv.multiplex

description: signal multiplexer

categories:
- object

pdcategory: heavylib, Mixing and Routing

arguments:

inlets:
  1st:
  - type: signal
    description: signal a
  2nd:
  - type: signal
    description: signal b
  3rd:
  - type: float/signal
    description: left-hand side
  4th:
  - type: float/signal
    description: right-hand side

outlets:
  1st:
  - type: signal
    description: signal a if lhs > rhs, signal b otherwise

draft: false
---

