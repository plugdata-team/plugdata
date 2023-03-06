---
title: change
description: remove repeated numbers from a stream
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.27'
arguments:
- description: initial value 
  default: 0
  type: float
inlets:
  1st:
  - type: bang
    description: output current value
  - type: float
    description: input value (repeated numbers are filtered)
outlets:
  1st:
  - type: float
    description: unrepeated value
methods:
  - type: set <float>
    description: set the value
draft: false
---

