---
title: togedge

description: report zero / non-zero transitions

categories:
 - object

pdcategory: General

arguments: (none)

inlets:
  1st:
  - type: float
    description: number to check for transitions
  - type: bang
    description: switches the stored value between 0 and non-zero

outlets:
  1st:
  - type: bang
    description: if a zero to non-zero transition is detected
  2nd:
  - type: bang
    description: if a non-zero to zero transition is detected

draft: true
---

[togedge] sends a bang in the left outlet for "zero to non-zero" transitions, and a bang in the right outlet for "non-zero to zero" transitions.