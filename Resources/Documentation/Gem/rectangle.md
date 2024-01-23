---
title: rectangle
description: renders a rectangle
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: draw [line|fill|point]
    default: fill
  - type: float
    description: width (default to 1)
    default: 1
  - type: float
    description: height (default to 1)
    default: 1
methods:
  - type: draw [line|fill|point]
    description:
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: width (default to 1)
  3rd:
    - type: float
      description: height (default to 1)
outlets:
  1st:
    - type: gemlist
draft: false
---
