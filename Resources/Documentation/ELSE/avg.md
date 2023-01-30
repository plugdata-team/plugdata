---
title: avg

description: mean average

categories:
- object

pdcategory: ELSE, Data Math

arguments:

inlets:
  1st:
  - type: float
    description: number to add to the moving average
outlets:
  1st:
  - type: float
    description: the average so far
  2nd:
  - type: float
    description: total count of values processed

methods:
  - type: clear
    description: clears memory (previously received numbers)

draft: false
---

[avg] calculates the mean average of all received numbers so far. A clear message resets the object.
