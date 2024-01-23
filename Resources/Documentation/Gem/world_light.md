---
title: world_light
description: adds a point-light to the scene.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: turn light on/off (default: 1)
inlets:
  1st:
    - type: gemlist
      description: debug 1|0: show debugging object (default: 0), f 66
  2nd:
    - type: list
      description: 3(RGB) or 4(RGBA) float values
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
