---
title: light
description: adds a point-light to the scene.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: gemlist
    description:
methods:
  - type: debug <float>
    description: turn debug mode on/off.
  - type: list <float> <float> <float> <float>
    description: set RGB or RGBA float values for the light color.
inlets:
  1st:
    - type: float
      description: turn light on/off.
  2nd:
    - type: list
      description: RGB or RGBA float values for the light color.
outlets:
  1st:
    - type: gemlist
draft: false
---
