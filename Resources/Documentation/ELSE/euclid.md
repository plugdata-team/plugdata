---
title: euclid

description: Euclidean rhythm algorithm

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: number of steps
  type: float
  default: 8
- description: number of hits
  type: float
  default: 1
- description: rotation value
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: step number
  2nd:
  - type: float
    description: number of steps
  3rd:
  - type: float
    description: number of hits
  4th:
  - type: float
    description: rotation value

outlets:
  1st:
  - type: bang
    description: when there's a hit
  2nd:
  - type: bang
    description: when there's a rest
  3rd:
  - type: float
    description: 1 for hit, 0 for rest

draft: false
---

[euclid] implements an Euclidean rhythm algorithm. It takes a step number and returns if that step is a hit or a rest depending on parameters of: number of steps, number of hits and rotation value.

