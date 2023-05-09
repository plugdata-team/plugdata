---
title: bitnormal~

description: replace NaN/Inf and denormal signals with 0

categories:
 - object

pdcategory: ELSE, Signal Math, Filters

arguments:
inlets:
  1st:
  - type: signal
    description: input whose NaN/Inf/denormal values are replaced with 0
  
outlets:
  1st:
  - type: signal
    description: the filtered signal

draft: false
---

[bitnormal~] replaces NaN (not a number), +/- infinity values and denormals of an incoming signal with zero (hence, it only lets 'normal' floats through. This is useful in conjunction with the bitwise operators in [op~], [expr~] or any other situation where these values are possible.
