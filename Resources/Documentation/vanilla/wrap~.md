---
title: wrap~
description: remainder modulo 1 for signals
categories:
- object
pdcategory: vanilla, Signal Math
last_update: '0.48'
see_also:
- wrap
- expr~
inlets:
  1st:
  - type: signal
    description: input to 'modulo 1' function
outlets:
  1st:
  - type: signal
    description: output of 'modulo 1' function
draft: false
---
wrap~ gives the difference between the input and the largest integer not exceeding it (for positive numbers this is the fractional part)

COMPATIBILITY NOTE: in Pd versions before 0.48, wrap~ with an input of 0 output 1 (but now correctly outputs 0). To get the old behavior, set "compatibility" to 0.47 or below in Pd's command line or by a message

