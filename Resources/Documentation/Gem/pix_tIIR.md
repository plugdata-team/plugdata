---
title: pix_tIIR
description: timebased IIR-filter
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: IIR coefficients
methods:
    - type: set
      description: overwrites the filter-buffers with the next incoming image, zero clears buffer
inlets:
  1st:
    - type: gemlist
      description:
  nth:
    - type: float
      description: $nth feedback-coefficient
outlets:
  1st:
    - type: gemlist
draft: false
---
