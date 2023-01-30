---
title: bitsafe~
description: replace nan/inf and denormal signals with 0
categories:
 - object
pdcategory: cyclone, General
arguments:
inlets:
  1st:
  - type: signal
    description: input signals to have nan/inf values replaced with 0
outlets:
  1st:
  - type: signal
    description: the signal which has 0 values where nan/inf values existed

---

[bitsafe~] replaces NaN (not a number) and +/- infinity values of an incoming signal with zero, which is useful in conjunction with the bitwise operators in cyclone or any other situation where these values are possible. Also, bitsafe~ in cyclone filters out denormals and turns them to zero, even though the original MAX object doesn't.

