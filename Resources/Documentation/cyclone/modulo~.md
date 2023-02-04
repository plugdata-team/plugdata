---
title: modulo~, %~, cyclone/%~
description: modulo for signals
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: a value by which to divide the incoming signal
  default: 0
inlets:
  1st:
  - type: signal
    description: input to modulo operation
  2nd:
  - type: float/signal
    description: a value by which to divide the incoming signal
outlets:
  1st:
  - type: signal
    description: the modulo of the operation (remainder of the division)

---

[modulo~] or [%~] is a signal remainder operator. The left signal is divided by the right inlet input (or argument), and the remainder is output.

