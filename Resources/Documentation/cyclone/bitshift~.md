---
title: bitshift~
description: signal bit-shifting
categories:
 - object
pdcategory: cyclone, Logic, Signal Math
arguments:
- type: float
  description: number of bits to shift
  default: 0
- type: float
  description: sets conversion mode: 0 or 1
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to Bit-Shift
  - type: float
    description: sets conversion mode [0 or 1]
outlets:
  1st:
  - type: signal
    description: output of bit shifted signal

methods:
  - type: mode <0/1>
    description: sets mode (details in help)
  - type: shift <float>
    description: specifies number and direction of shift - positive values shift that number of bits to the left while negative shift to the right

draft: false
---

[bitshift~] takes a signal and performs Bitwise Shifting (shift bit values to the left or right) depending on a shift value (positive values shift that number of bits to the left, while negative values shift to the right). The operation is done in two modes (see help file).

