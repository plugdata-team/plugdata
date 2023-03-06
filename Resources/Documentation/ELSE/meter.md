---
title: meter

description: CPU meter

categories:
- object

pdcategory: ELSE, UI

arguments:
- type: float
  description: 0 - off, 1 - on
  default: 1

inlets:
  1st:
  - type: float
    description: 0 - off, 1 - on

outlets:
  1st:
  - type: float
    description: CPU load value

draft: false
---

[meter] is a CPU meter based on the "Load Meter" patch found in Pd's Media menu.
