---
title: gatedelay

description: delay a gate on value

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description:  delay time in ms
  default: 0

inlets:
  1st:
  - type: float
    description: gate value
  2nd:
  - type: float
    description: delay time in ms

outlets:
  1st:
  - type: float
    description: delayed gate

draft: false
---

[gatedelay] delays a control gate on value, but the gate off is immediately output.
