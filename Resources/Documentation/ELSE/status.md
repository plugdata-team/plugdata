---
title: status

description: report 0/non-0 transitions

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: initial status value
  type: float
  default: no status

inlets:
  1st:
  - type: float
    description: number to check for transitions
  - type: bang
    description: resends bang according to current status

outlets:
  1st:
  - type: bang
    description: if a zero to non-0 transition is detected
  2nd:
  - type: bang
    description: if a non-0 to zero transition is detected

methods:
  - type: change
    description: changes internal status

draft: false
---

[status] sends a bang in the left outlet for "zero to non-0" transitions and a bang in the right outlet for "non-0 to zero" transitions.
