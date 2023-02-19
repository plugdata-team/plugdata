---
title: mean
description: moving mean average
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
inlets:
  1st:
  - type: bang
    description: resends the most recent output
  - type: float
    description: number to add to the moving average
  - type: list
    description: clears memory and calculates the mean from the list
outlets:
  1st:
  - type: float
    description: the moving average
  2nd:
  - type: float
    description: total number of values processed

methods:
  - type: clear
    description: clears memory (previously received numbers)

draft: false
---

[mean] calculates a running/moving average of all received numbers (the sum of all received values by the number of received elements).

