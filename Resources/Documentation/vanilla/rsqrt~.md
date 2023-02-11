---
title: rsqrt~
description: signal reciprocal square root
categories:
- object
pdcategory: vanilla, Signal Math
last_update: '0.47'
aliases:
- q8_rsqrt~
see_also:
- sqrt
- expr~
inlets:
  1st:
  - type: signal
    description: input to reciprocal square root function
outlets:
  1st:
  - type: signal
    description: output of reciprocal square root function
draft: false
---
rsqrt~ takes the approximate reciprocal square root of the incoming signal (the same as '1/sqrt(input) using a fast approximate algorithm which is probably accurate to about 120 dB (20 bits)

An older object, q8_rsqrt~, is included in Pd for back compatibility but should probably not be used. It only gives about 8 bit accuracy.
