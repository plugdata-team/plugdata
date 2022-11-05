---
title: status

description: Report 0/non-0 transitions

categories:
- object

pdcategory: Control (Triggers)

arguments:
- description: initial status value (default no status)
  type: float
  default: no status

inlets:
  1st:
  - type: float
    description: number to check for transitions
  - type: bang
    description: resends bang according to current status
  - type: change
    description: changes internal status

outlets:
  1st:
  - type: bang
    description: if a zero to non-zero transition is detected
  2nd:
  - type: bang
    description: if a non-zero to zero transition is detected

draft: false
---

[status] sends a bang in the left outlet for "zero to non-zero" transitions and a bang in the right outlet for "non-zero to zero" transitions.
