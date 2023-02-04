---
title: sinx~

description: signal sine function (radian input)

categories:
 - object

pdcategory: cyclone, Signal Math

arguments: (none)

inlets: 
  1st:
  - type: signal
    description: input to sine function (in radians)

outlets:
  1st:
  - type: signal
    description: output of sine function

draft: true
---

Like [cosx~] & [tanx~], [sinx~] is properly designed for mathematical operations. Thus, it expects an input in radians to calculate the sine of each input sample.
