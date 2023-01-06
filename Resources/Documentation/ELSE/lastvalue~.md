---
title: lastvalue~
description: Report last value

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: initial last value
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to detect the last value
  2nd:
  - type: float
    description: set new last value

outlets:
  1st:
  - type: signal
    description: last input value

---

[lastvalue~] reports the last value when the signal changes

