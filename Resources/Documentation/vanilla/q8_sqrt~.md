---
title: q8_sqrt~
description: signal square root
categories:
- object
pdcategory: Audio Math
last_update: '0.47'
see_also:
- sqrt~
- rsqrt~
- sqrt
- exp~
- expr~
- log~
- pow~
inlets:
  1st:
  - type: signal
    description: input to square root function.
outlets:
  1st:
  - type: signal
    description: output of square root function.
draft: false
---
q8_sqrt~ is included in Pd for back compatibility but should probably not be used. It only gives about 8 bit accuracy.

Use [sqrt~].