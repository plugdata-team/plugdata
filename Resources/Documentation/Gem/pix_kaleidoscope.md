---
title: kaleidoscope
description: kaleidoscope effect
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: number of segments (0-64)
  3rd:
    - type: float
      description: rotation of the input-segment in degrees
  4th:
    - type: list
      description: normalized x and y centre coordinates of the of the segment of the input image. (0-1)
  5th:
    - type: float
      description: rotation of the output-segment (in degrees)
  6th:
    - type: list
      description: normalized x and y centre coordinates of the of the segments in the output image. (0-1)
  7th:
    - type: float
      description: reflection line proportion, controls the relative sizes of each pair of adjacent segments in the output image (0-1)
  8th:
    - type: float
      description: source angle proportion (0.1-10)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
