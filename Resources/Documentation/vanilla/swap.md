---
title: swap, fswap
description: swap two numbers
categories:
- object
aliases:
- fswap
pdcategory: vanilla, Mixing and Routing
last_update: '0.41'
arguments:
- description: initial right inlet value 
  default: 0
  type: float
inlets:
  1st:
  - type: bang
    description: outputs the stored values swapped
  2nd:
  - type: float
    description: set left value, swap and output
outlets:
  1st:
  - type: float
    description: value from right/2nd inlet
  2nd:
  - type: float
    description: value from left/1st inlet
draft: false
---
The swap object swaps the positions of two incoming numbers. The number coming in through the right inlet will be sent to the left outlet and the number coming in left will come out right. Only the left inlet is hot and triggers output on both outlets. Output order is right to left as in [trigger].
