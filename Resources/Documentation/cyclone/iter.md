---
title: iter
description: split messages sequentially
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
inlets:
  1st:
  - type: bang
    description: outputs the last received input as sequential elements
  - type: anything
    description: split elements sequentially
outlets:
  1st:
  - type: float/symbol
    description: according to the input element, in sequential order

draft: false
---

[iter] is similar to [unnpack], it splits a message (to floats/symbols) but outputs them sequentially in the given order.

