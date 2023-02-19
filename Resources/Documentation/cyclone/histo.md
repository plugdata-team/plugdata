---
title: histo
description: create a histogram of numbers
categories:
 - object
pdcategory: cyclone, Data Math, Analysis
arguments:
- type: float
  description: sets the histogram size
  default: 128 (0-127)
inlets:
  1st:
  - type: bang
    description: repeat the last output or input?
  - type: float
    description: input integer values to be stored (from 0 to 'size - 1')
  2nd:
  - type: float
    description: check how many times this number was received
outlets:
  1st:
  - type: float
    description: the input value
  2nd:
  - type: float
    description: the number of times the value was received

methods:
  - type: clear
    description: erases the received numbers

draft: false
---

[histo] records how many times it received a positive integer (no floats allowed). An input number is sent out the left outlet and its occurrence at the right outlet. A number in the right inlet checks and outputs that number's histogram.

