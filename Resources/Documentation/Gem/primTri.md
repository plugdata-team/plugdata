---
title: primTri
description: renders a triangle with gradient colors
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: symbol
    description: draw [line|fill|point]
    default: fill
methods:
  - type: draw [line|fill|point]
    description:
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: 3(XYZ) float values (position of 1st corner)
  3rd:
    - type: list
      description: 3(XYZ) float values (position of 2nd corner)
  4th:
    - type: list
      description: 3(XYZ) float values (position of 3rd corner)
  5th:
    - type: list
      description: 3(RGB) or 4(RGBA) float values (color of 1st corner)
  6th:
    - type: list
      description: 3(RGB) or 4(RGBA) float values (color of 2nd corner)
  7th:
    - type: list
      description: 3(RGB) or 4(RGBA) float values (color of 3rd corner)
outlets:
  1st:
    - type: gemlist
draft: false
---
