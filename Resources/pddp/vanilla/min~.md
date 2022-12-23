---
title: min~
description: binary operators on audio signals
categories:
- object
pdcategory: Audio Math
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
- description: initialize value of right inlet and makes it only take floats instead
    of signals 
  default: 0
  type: float
inlets:
  1st:
  - type: signal
    description: Set value on left-hand side and trigger output
  2nd:
  - type: float/signal
    description: Set value on right-hand side
outlets:
  1st:
  - type: signal
    description: The result of the operation.
draft: false
---
This object combine two signals as above, or, if you give a numeric argument, the right inlet only takes floats (no signals