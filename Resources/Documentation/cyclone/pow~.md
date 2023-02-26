---
title: cyclone/pow~

description: signal power function

categories:
 - object

pdcategory: cyclone, Signal Math

arguments: 
  - type: float
    description: sets base value
    default: 0

inlets:
  1st:
  - type: float/signal
    description: sets exponent
  2nd:
  - type: float/signal
    description: sets the base value

outlets:
  1st:
  - type: signal
    description: the base raised to the exponent

draft: false
---

[cyclone/pow~] has the inlets'functionality reversed in comparison to pd vanilla's [pow~], other than that, it's quite the same! [cyclone/pow~] raises the base value (set in the right inlet) to the power of the exponent (set in the left inlet). Either inlet can receive a signal or float.[cyclone/pow~] can be useful for generating curves from [line~] as in the example below to the right.