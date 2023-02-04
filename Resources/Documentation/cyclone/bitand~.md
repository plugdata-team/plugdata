---
title: bitand~
description: signal Bitwise-AND
categories:
 - object
pdcategory: cyclone, Logic, Signal Math
arguments:
- type: float
  description: converted to integer and used as bitmask
  default: 0
- type: float
  description: mode [0-3] (details in help)
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to execute Bitwise-AND on
  2nd:
  - type: signal
    description: signal to execute Bitwise-AND on
  - type: float
    description: converted to integer and used as bitmask
outlets:
  1st:
  - type: signal
    description: signal as result of the "Bitwise-AND" operation

methods:
  - type: bits <list>
    description: bits plus 32 bit (0/1) values sets a raw 32-bits bitmask
  - type: mode <float>
    description: <0-3>: modes of conversion to integers (details in help)

---

[bitand~] compares the bits of two values with "Bitwise-AND" (bits are set to 1 if both are "1", 0 otherwise). It compares two signals or a signal to a given bitmask. It has 4 modes of comparison (see help file).

