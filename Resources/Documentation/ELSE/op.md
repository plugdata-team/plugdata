---
title: op

description: float/list operators

categories:
- object

pdcategory: ELSE, Data Math, Data Management

arguments:
  - description: operator: see [pd set] for all options
    type: symbol
    default: +
  - description: inlet value - ignored for bitnot
    type: float
    default: 0

inlets:
  1st:
  - type: float/list
    description: input to operator
  2nd:
  - type: float
    description: secondary operator value (ignored for bitnot)

outlets:
  1st:
  - type: float/list
    description: operator result

methods:
  - type: <symbol>
    description: operator see [pd set] for all options

draft: false
---

[op] provides many operators and is most useful for lists, as there are objects in Pd Vanilla for all these operations.
