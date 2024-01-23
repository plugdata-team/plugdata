---
title: pix_background
description: separate an object from the background
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: reset
    description: reset the background and capture a new image
inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: alias for reset
  3rd:
    - type: float
      description: greyscale range
    - type: list
      description: range
outlets:
  1st:
    - type: gemlist
draft: false
---
