---
title: scale
description: scales a gemlist with specified scaling factors
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: initial scale amount
  - type: list
    description: scaling vector (X Y Z)
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: scaling amount along the vector
  3rd:
    - type: list
      description: scaling vector (X Y Z)
outlets:
  1st:
    - type: gemlist
draft: false
---
