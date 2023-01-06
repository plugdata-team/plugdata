---
title: trighold~

description: Hold a trigger value

categories:
- object

pdcategory: General

arguments:
- description:
  type:
  default:

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

[trighold~] holds a trigger value. A trigger happens when a transition from 0 to non-zero occurs.