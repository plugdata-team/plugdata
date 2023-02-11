---
title: sqrt~
description: signal square root
categories:
- object
pdcategory: vanilla, Signal Math
last_update: '0.47'
aliases:
- q8_sqrt~
see_also:
- rsqrt~
- sqrt
- exp~
- expr~
- log~
- pow~
inlets:
  1st:
  - type: signal
    description: input to square root function
outlets:
  1st:
  - type: signal
    description: output of square root function
draft: false
---
sqrt~ takes the approximate square root of the incoming signal, using a fast, approximate algorithm which is probably accurate to about 120 dB (20 bits).

An older object, q8_sqrt~, is included in Pd for back compatibility but should probably not be used. It only gives about 8 bit accuracy.
