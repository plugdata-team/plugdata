---
title: spline_path
description: reads out a table
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: reading point (0.0 to 1.0)
  - type: float
    description: dimensions of the table
    default: 1
  - type: symbol
    description: name of the table
inlets:
  1st:
    - type: float
      description: reading point (0.0 to 1.0)
outlets:
  1st:
    - type: list of floats
      description: a list of <D> floats
draft: false
---
