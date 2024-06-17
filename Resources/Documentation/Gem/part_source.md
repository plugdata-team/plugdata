---
title: part_source
description: adds a particle-source to a particle system
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: number of particles emitted at each rendering frame
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