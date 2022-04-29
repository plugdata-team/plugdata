---
title: q8_rsqrt~
description: signal reciprocal square root
categories:
- object
pdcategory: Audio Math
last_update: '0.47'
see_also:
- rsqrt~
- sqrt
- expr~
inlets:
  1st:
  - type: signal
    description: input to reciprocal square root function.
outlets:
  1st:
  - type: signal
    description: output of reciprocal square root function.
draft: false
---
q8_rsqrt~, is included in Pd for back compatibility but should probably not be used. It only gives about 8 bit accuracy.

Use [rsqrt~].