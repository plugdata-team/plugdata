---
title: bitnormal~

description: Biquad Series

categories:
 - object

pdcategory: Audio Math

arguments: (none)

inlets:
  1st:
  - type: signal
    description: input whose nan/inf/denormal values are replaced with 0
  
outlets:
  1st:
  - type: signal
    description: the filtered signal

draft: false
---

[bitnormal~] replaces NaN (not a number), +/- infinity values and denormals of an incoming signal with zero (hence, it only lets 'normal' floats through. This is useful in conjunction with the bitwise operators in [op~], [expr~] or any other situation where these values are possible.