---
title: lastvalue

description: report last value

categories:
- object

pdcategory: ELSE, Analysis

arguments:
- description: initial last value
  type: float
  default: none

inlets:
  1st:
  - type: float
    description: stores value and outputs last value
  - type: bang
    description: outputs last stored value
  2nd:
  - type: float
    description: sets last value
  - type: bang
    description: sets last value to none

outlets:
  1st:
  - type: float
    description: last input value

methods:
  - type: reset <float>
    description: resets object and sets last value, no float sets to none

draft: false
---

[lastvalue] reports the last input value

