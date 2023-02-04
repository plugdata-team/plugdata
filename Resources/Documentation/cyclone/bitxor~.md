---
title: bitxor~
description: signal Bitwise-XOR
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
    description: signal to execute Bitwise-XOR on
  2nd:
  - type: signal
    description: signal to execute Bitwise-XOR on
  - type: float
    description: converted to integer and used as bitmask
outlets:
  1st:
  - type: signal
    description: signal as result of the "Bitwise-XOR" operation

flags:
  - name: @mode <float>
    description: sets conversion mode (default 0)

methods:
  - type: bits <list>
    description: bits plus 32 bit (0/1) values sets a raw 32-bits bitmask
  - type: mode <float>
    description: <0-3>: modes of conversion to integers (details in help)

---

[bitxor~] compares the bits of two values with "Bitwise eXclusive OR" (bits are set to 1 if different, 0 otherwise). It compares two signals or a signal to a given bitmask. It has 4 modes of comparison (see below).

