---
title: smooth

description: control smoothner

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: glide time in ms
  default: 0

flags:
  - type: -curve <float>
    description: sets curve factor (default '0', linear)
  - type: -rate <float>
    description: sets refresh rate in ms (default 5, minimum 1)

methods:
  - type: reset
    description: resets to input value
  - type: curve <float>
    description: sets curve factor

inlets:
  1st:
  - type: float
    description: control input
  2nd:
  - type: float
    description: smooth time in ms

outlets:
  1st:
  - type: float
    description: smoothned control

draft: false
---

[smooth] smoothens a control transition with linear interpolation by default, at a given time in ms. You can also have exponential curves.
