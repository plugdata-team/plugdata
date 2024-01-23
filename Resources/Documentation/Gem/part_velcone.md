---
title: part_velcone
description: sets a cone with a specified height and center to be the velocity-domain of emitted particles
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: default dimensions (x y z height)
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: list
      description: x, y, z coordinates
  3rd:
    - type: float
      description: height
outlets:
  1st:
    - type: gemlist
draft: false
---
