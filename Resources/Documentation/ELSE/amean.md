---
title: amean

description: generate list with arithmetic means

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
  - description: sets step-count
    type: float
    default: 2

inlets:
  1st:
  - type: bang
    description: generate list
  - type: list
    description: set start/end/step-count and generate list
  2nd:
  - type: float
    description: sets start
  3rd:
  - type: float
    description: sets end
  4th:
  - type: float
    description: sets step-count

outlets:
  1st:
  - type: list
    description: list with arithmetic means

draft: false
---

[amean] creates a list of arithmetic means. It takes a start point, an end point and the number of steps from start to end. The number of steps is the output list length -1 and represents the number of values until reaching the end. The minimum number of steps is "1".
