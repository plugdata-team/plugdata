---
title: specularRGB
description: specular coloring with individual RGB values
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: a list of 3 (RGB) or 4 (RGBA) float-values
    default: 0 0 0 1
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: red value
  3rd:
    - type: float
      description: green value
  4th:
    - type: float
      description: blue value
  5th:
    - type: float
      description: alpha value
outlets:
  1st:
    - type: gemlist
draft: false
---
