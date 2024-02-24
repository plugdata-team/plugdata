---
title: pix_duotone
description: reduce the number of colours by thresholding
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
      description: gemlist
  2nd:
    - type: list
      description: threshold (RGB)
  3rd:
    - type: list
      description: color if original pixel-value is below threshold
  4th:
    - type: list
      description: color if original pixel-value is above threshold
outlets:
  1st:
    - type: gemlist
draft: false
---
