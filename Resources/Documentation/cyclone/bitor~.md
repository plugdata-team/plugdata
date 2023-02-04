---
title: bitor~
description: signal Bitwise-OR
categories:
 - object
pdcategory: cyclone, Logic, Signal Math
arguments:
- type: float
  description: converted to integer and used as bitmask
  default: 0
- type: float
  description: mode [0-3] (details above)
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to execute Bitwise-OR on
  2nd:
  - type: signal
    description: signal to execute Bitwise-OR on
  - type: float
    description: converted to integer and used as bitmask
outlets:
  1st:
  - type: signal
    description: signal as result of the "Bitwise-OR" operation

methods:
  - type: bits <list>
    description: bits plus 32 bit (0/1) values sets a raw 32-bits bitmask
  - type: mode <float>
    description: <0-3>: modes of conversion to integers (details in help)

---

[bitor~] compares the bits of two values with "Bitwise OR" (bits are set to 1 if any of them is "1", 0 otherwise). It compares two signals or a signal to a given bitmask. TIt has 4 modes of comparison (see help file).

