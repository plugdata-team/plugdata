---
title: part_velsphere
description: sets a sphere with a specified radius and centre to be the velocity-domain of emitted particles
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: default dimensions (x y z radius)
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: list
      description: x, y, z coordinates
  3rd:
    - type: float
      description: radius
outlets:
  1st:
    - type: gemlist
draft: false
---
