---
title: polygon
description: Renders a polygon.
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
      description: List of X Y Z points that define the polygon
  2nd and onwards:
    - type: list
      description: "3(XYZ) float values representing control points of the polygon"
outlets:
  1st:
    - type: gemlist
      description: Rendered polygon
draft: false
---
