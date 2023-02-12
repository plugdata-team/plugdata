---
title: deg2rad

description: degrees to radians conversion

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:
- description: initial degree value
  type: float
  default: 0

inlets:
  1st:
  - type: bang
    description: convert the last value
  - type: float
    description: input degree value to convert to radians

outlets:
  1st:
  - type: float
    description: the converted value in radians

flags:
  - name: -pos
    description: wrap to positive only (default: allow also negative)

draft: false
---

[deg2rad] converts degrees (0 to 360) to radians (0 to 2*pi). You can also convert negative values from 0 to -360 (0 to -2*pi), unless you give it the -pos, so values are wrapped to positive. Values outside the input range are wrapped.

