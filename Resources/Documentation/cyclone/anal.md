---
title: anal
description: number pairs histogram
categories:
 - object
pdcategory: cyclone, Data Math, Random and Noise
arguments:
- type: float
  description: maximum input (max 16384)
  default: 128
inlets:
  1st:
  - type: float
    description: the anal input data
outlets:
  1st:
  - type: list <f, f, f>
    description: <previous number, current number, occurrence>

methods:
  - type: clear
    description: clears analysis but remembers last input
  - type: reset
    description: completely erase memory

draft: false
---

[anal] reports how many times it received a number pair. The output is designed to serve as input to [prob] and is composed of: 1) the previously received number, 2) current input number, 3) occurrence (how many times this number pair has been received).

