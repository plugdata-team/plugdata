---
title: tanx~

description: signal tangent function (radian input)

categories:
 - object

pdcategory: cyclone, Signal Math

arguments: (none)

inlets:
  1st:
  - type: signal
    description: input to tangent function (in radians)

outlets:
  1st:
  - type: signal
    description: tangent of the input

draft: true
---

Like [cosx~] & [sinx~], [tanx~] is properly designed for mathematical operations. Thus, it expects an input in radians to calculate the tangent of each input sample.
