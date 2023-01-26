---
title: status~

description: report 0/non-0 transitions

categories:
 - object
 
pdcategory: Triggers and Clocks

arguments:
- type: float
  description: initial status value 
  default: 0
  
inlets:
  1st:
  - type: signal
    description: signal to detect transitions from

outlets:
  1st:
  - type: signal
    description: impulse if a zero to non-zero transition is detected
  2nd:
  - type: signal
    description: impulse if a non-zero to zero transition is detected

draft: false
---

[status~] is a signal version of [status]. It sends an impulse in the left outlet for "zero to non-zero" transitions and an impulse in the right outlet for "non-zero to zero" transitions.