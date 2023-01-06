---
title: avg

description: Mean average

categories:
- object

pdcategory: Math

arguments:
- description: none
  type:
  default:

inlets:
  1st:
  - type: float
    description: number to add to the moving average
  - type: 
    description: clears memory (previously received numbers)
outlets:
  1st:
  - type: float
    description: the average so far
  2nd:
  - type: float
    description: total count of values processed

draft: false
---

[avg] calculates the mean average of all received numbers so far. A clear message resets the object.