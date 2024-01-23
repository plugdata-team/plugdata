---
title: pix_backlight
description: radially displace pixels depending on their luminance value, producing a backlighting effect
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: scale-factor
    default: 0.5
  - type: float
    description: floor
    default: 0
  - type: float
    description: ceiling
    default: 1
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: float
      description: scale-factor
  3rd:
    - type: float
      description: floor
  4th:
    - type: float
      description: ceiling
outlets:
  1st:
    - type: gemlist
draft: false
---
