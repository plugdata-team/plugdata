---
title: pix_blob
description: calculate the "center-of-gravity" of a certain (combination of) channel(s)
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: float
      description: mode (0=gray, 1=red, 2=green, 3=blue, 4=alpha)
  3rd:
    - type: list
      description: colour weights
outlets:
  1st:
    - type: gemlist
draft: false
---
