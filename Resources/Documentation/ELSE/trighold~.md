---
title: trighold~

description: hold a trigger value

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:

inlets:
  1st:
  - type: signal
    description: trigger signal to be held
  - type: clear
    description: clear and set output to zero

outlets:
  1st:
  - type: signal
    description: held trigger

draft: false
---

[trighold~] holds a trigger value. A trigger happens when a transition from 0 to non-0 occurs.
