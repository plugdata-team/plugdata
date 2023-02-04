---
title: framedelta~
description: FFT frame deviation
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
inlets:
  1st:
  - type: signal
    description: The input on which the deviation will be computed
outlets:
  1st:
  - type: signal
    description: Deviation between successive FFT frames

---

The [framedelta~] object was designed specially to compute a running phase deviation of a FFT analysis by subtracting values in each position of its previously received signal block from the current signal block. When used in a FFT subpatch, it can keep a running phase deviation of the FFT because the FFT size is equal to the block size.

