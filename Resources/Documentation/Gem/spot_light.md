---
title: spot_light
description: Adds a spot-light to the scene
categories:
- object
pdcategory: Gem, Graphics
arguments:
- type: float
  description: Turn light on/off
  default: 1
- type: debug <float>
  description: Show debugging object
- type: list
  description: Set the color of the light-source
- type: list
  description: Set the attenuation parameters
inlets:
 1st:
    - type: float
    description: Turn light on/off
 2nd:
    - type: list
    description: Set the color of the light-source
 3rd:
    - type: list
    description: Set the attenuation parameters
outlets:
  1st:
    - type: gemlist
    description: None
draft: false
---