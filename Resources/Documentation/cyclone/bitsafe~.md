---
title: bitsafe~
description: replace NaN/Inf and denormal signals with 0
categories:
 - object
pdcategory: cyclone, Logic, Signal Math
arguments:
inlets:
  1st:
  - type: signal
    description: input signals to have NaN/Inf values replaced with 0
outlets:
  1st:
  - type: signal
    description: the signal which has 0 values where NaN/Inf values existed

---

[bitsafe~] replaces NaN (not a number) and +/- infinity values of an incoming signal with zero, which is useful in conjunction with the bitwise operators in cyclone or any other situation where these values are possible. Also, bitsafe~ in cyclone filters out denormals and turns them to zero, even though the original MAX object doesn't.

