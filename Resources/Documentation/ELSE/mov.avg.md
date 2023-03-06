---
title: mov.avg

description: moving average

categories:
- object

pdcategory: ELSE, Analysis, Data Math

arguments:
- type: float
  description: sets initial values
  default: 1

inlets:
  1st:
  - type: float
    description: value into the moving average
  2nd:
  - type: float
    description: sets new number of values and clears

outlets:
  1st:
  - type: float
    description: the moving average

methods:
  - type: clear
    description: clears all received values

draft: false
---

[mov.avg] gives you a running/moving average over the last 'n' values.
