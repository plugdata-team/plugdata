---
title: rad2deg

description: radians to degrees conversion

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: initial radians value
  default: 0

inlets:
  1st:
  - type: float
    description: input radians value
  - type: bang
    description: convert the last value

outlets:
  1st:
  - type: float
    description: converted value in degrees

flags:
  - name: -pos
    description: wraps to positive values only

draft: false
---

[rad2deg] converts radians (0 to 2*pi) to radians (0 to 360). Besides 0 to 2*pi you can also convert negative values from 0 to -2*pi (0 to -360) - and input values are wrapped if the exceed this input range. If you give it the '-pos' flag, which wraps to positive values only.
