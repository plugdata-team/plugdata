---
title: max~
description: binary operators on audio signals
categories:
- object
pdcategory: vanilla, Signal Math
last_update: '0.27'
see_also:
- +
- cos~
- wrap~
- abs~
- log~
- sqrt~
- pow~
- expr~
arguments:
- description: initialize value of right inlet and makes it only take floats
  default: 0
  type: float
inlets:
  1st:
  - type: signal
    description: set value on left-hand side and trigger output
  2nd:
  - type: float/signal
    description: set value on right-hand side
outlets:
  1st:
  - type: signal
    description: the result of the operation
draft: false
---
This object combine two signals as above, or, if you give a numeric argument, the right inlet only takes floats (no signals
