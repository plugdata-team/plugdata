---
title: part_sink
description: sets up a sink for the particles within the system
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: domain, one of "point", "line", "triangle", "plane", "box", "sphere", "cylinder", "cone", "blob", "disc", "rectangle"
  - type: list
    description: up to 9 floats defining the specified domain
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: symbol
      description: domain, one of "point", "line", "triangle", "plane", "box", "sphere", "cylinder", "cone", "blob", "disc", "rectangle"
  3rd:
    - type: list
      description: up to 9 floats defining the specified domain
outlets:
  1st:
    - type: gemlist
draft: false
---
