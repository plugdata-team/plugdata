---
title: togedge

description: report zero / non-0 transitions

categories:
 - object

pdcategory: cyclone, Data Management, Analysis

arguments: (none)

inlets:
  1st:
  - type: float
    description: number to check for transitions
  - type: bang
    description: switches the stored value between 0 and non-0

outlets:
  1st:
  - type: bang
    description: if a zero to non-0 transition is detected
  2nd:
  - type: bang
    description: if a non-0 to zero transition is detected

draft: true
---

[togedge] sends a bang in the left outlet for "zero to non-0" transitions, and a bang in the right outlet for "non-0 to zero" transitions.
