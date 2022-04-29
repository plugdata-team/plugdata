---
title: '*~'
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
    of signals (default 0).
  type: float
inlets:
  1st:
  - type: signal
    description: value to the left side of operation and output.
  2nd:
  - type: float/signal
    description: value to the right side of operation.
outlets:
  1st:
  - type: signal
    description: the result of the operation.
draft: false
---
This object combine two signals as above, or, if you give a numeric argument, the right inlet only takes floats (no signals) and the argument initializes the right inlet value.
