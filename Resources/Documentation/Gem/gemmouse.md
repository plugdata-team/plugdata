---
title: gemmouse
description: output mouse events in the GEM window
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: x-normalization
  - type: float
    description: y-normalization
inlets:
  1st:
    - type: non-used
outlets:
  1st:
    - type: float
      description: x position
  2nd:
    - type: float
      description: y position
  3rd:
    - type: float
      description: left button state
  4th:
    - type: float
      description: middle button state
  5th:
    - type: float
      description: right button state
draft: false
---
