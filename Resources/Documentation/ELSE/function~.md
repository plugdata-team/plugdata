---
title: function~
description: function generator

categories:
 - object

pdcategory: ELSE, Signal Math, Effects

arguments:
- type: list
  description: 3 or more floats to set the function
  default: no function

inlets:
  1st:
  - type: float/signal
    description: values from 0 to 1 reads function
  - type: list
    description: 3 or more floats set function

outlets:
  1st:
  - type: signal
    description: function output

flags:
  - name: -exp <list>
    description: sets function with an extra exponential element for each segment

methods:
  - type: exp <list>
    description: sets function with an extra exponential element for each segment
  - type: expl <list>
    description: sets exponential values for each line segment
  - type: expi <f, f>
    description: sets an exponential for a line segment specified by the first float indexed from 0

draft: false
---

[function~] generates functions from arguments/list input. Input values from 0 to 1 reads the function. It needs an odd number of elements in a list, staring with 3 values, the syntax is (point1, period, point2, period, point3, etc...). The overall sum of periods is normalized to 1!

