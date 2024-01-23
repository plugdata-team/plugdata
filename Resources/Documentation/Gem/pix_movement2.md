---
title: pix_movement2
description: time-based IIR-filter for motion detection
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: low threshold (0-1)
  - type: float
    description: initial threshold (0-1)

inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: set the current frame as "background"
  2nd:
    - type: float
      description: low threshold
  3rd:
    - type: float
      description: initial threshold
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
