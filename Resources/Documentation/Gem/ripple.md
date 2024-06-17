---
title: ripple
description: renders and distorts a square
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: message
    description: draw [line|fill|point]
    default: fill
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: size
  3rd:
    - type: bang
      description: grab
  4th:
    - type: float
      description: posX (centered)
  5th:
    - type: float
      description: posY (centered)
  6th:
    - type: float
      description: amount
outlets:
  1st:
    - type: gemlist
draft: false
---
