---
title: atan2~
description: signal arctan(a/b) function
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: value of "b" (not documented in Max)
inlets:
  1st:
  - type: signal
    description: "a" value input to an arc-tangent function
  2nd:
  - type: signal
    description: "b" value input to an arc-tangent function
outlets:
  1st:
  - type: signal
    description: result of arctan(a/b) in radians

draft: false
---

Use the [atan2~] object to output the arc-tangent of two given values ("a" and "b") calculated as: Arc-tangent (a/b).

