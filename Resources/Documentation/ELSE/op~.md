---
title: op~

description: signal operator

categories:
- object

pdcategory: ELSE, Signal Math

arguments:
  - description: operator. >, <, >=, <=, ==, !=, &&, ||, &, |, ~, ^, >>, <<, or %
    type: symbol
  - description: inlet value - ignored for bitnot
    type: float/signal
    default: 0

inlets:
  1st:
  - type: signal
    description: input to operator
  2nd:
  - type: float
    description: secondary operator value (ignored for bitnot)

outlets:
  1st:
  - type: signal
    description: operator result

methods:
  - type: <symbol>
    description: operator: >, <, >=, <=, ==, !=, &&, ||, &, |, ~, ^, >> and <<

draft: false
---

[op~] is a signal operator (either comparison or bitwise), which is defined by the first argument. This is more efficient than using [expr~] (which also contains all these operators).
