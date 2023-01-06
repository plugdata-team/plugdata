---
title: moses
description: part a numeric stream
categories:
- object
pdcategory: General
last_update: '0.33'
see_also:
- change
- select
arguments:
- description: set initial control value 
  default: 0
  type: float
inlets:
  1st:
  - type: float
    description: number to be parted.
  2nd:
  - type: float
    description: set control value.
outlets:
  1st:
  - type: float
    description: input number if less than control value.
  2nd:
  - type: float
    description: input number if equal or higher than control value.
draft: false
---
Moses takes numbers and outputs them at left if they're less than a control value,  and at right if they're greater or equal to it.
