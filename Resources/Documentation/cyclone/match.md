---
title: match
description: Output matching input
categories:
 - object
pdcategory: General
arguments:
- type: anything
  description: elements to match ('nn' = wildcard for any number)
inlets:
  1st:
  - type: anything
    description: one or more elements to look for a match
outlets:
  1st:
  - type: anything
    description: a message whose elements were matched

methods:
  - type: clear
    description: forget all received elements
  - type: set <anything>
    description: clears and sets new elements to match to

---

[match] sends data when the input matches the arguments.

