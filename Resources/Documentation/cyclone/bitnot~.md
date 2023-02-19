---
title: bitnot~
description: signal Bitwise-NOT
categories:
 - object
pdcategory: cyclone, Logic, Signal Math
arguments:
- type: float
  description: mode [0 or 1] (details in help)
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to perform "Bitwise NOT" on
  - type: float
    description: sets mode [0 or 1]
outlets:
  1st:
  - type: signal
    description: the resulting bit inverted signal

methods:
  - type: mode <0/1>
    description: sets mode (details in help)

draft: false
---

[bitnot~] takes a signal and performs Bitwise NOT operation, also known as bitwise inversion or one's complement (all bit values of 1 are set to 0 and vice versa). The operation is done in two modes (see help file).

