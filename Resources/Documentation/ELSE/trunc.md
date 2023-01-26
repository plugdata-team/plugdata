---
title: trunc

description: truncate to integer

categories:
- object

pdcategory: Data Math

arguments:
- description:
  type:
  default:

inlets:
  1st:
  - type: float/list
    description: input value(s) to trunc function

outlets:
  1st:
  - type: float/list
    description: output value(s) of trunc function

draft: false
---

[trunc] truncates floats and lists towards zero, that means only the integer value is considered, like in vanilla's [int].