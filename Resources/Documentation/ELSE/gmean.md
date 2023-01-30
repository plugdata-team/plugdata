---
title: gmean

description: generate list with geometric means

categories:
- object

pdcategory: ELSE, Data Math

arguments:
- description: sets start
  type: float
  default: 1
- description: sets end
  type: float
  default: 2
- description: sets steps
  type: float
  default: 2

inlets:
  1st:
  - type: bang
    description: generate list
  - type: list
    description: set start/end/steps and generate list
  2nd:
  - type: float
    description: sets start
  3rd:
  - type: float
    description: sets end
  4th:
  - type: float
    description: sets steps

outlets:
  1st:
  - type: list
    description: list with geometric means

draft: false
---

[gmean] creates a list of geometric means. It takes a start point, an end point and the number of steps from start to end.

