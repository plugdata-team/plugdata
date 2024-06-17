---
title: pix_levels
description: level adjustment
categories:
  - object
pdcategory: Gem, Graphics
methods:
    - type: uni <float>
      description: choose between uniform adjustment(1) or per-channel-adjustment (0)
    - type: inv <float>
      description: allow inversion of level-correction to avoid instabilities
    - type: auto <float>
      description: turns automatic level-correction on/off
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: inFloor inCeiling outFloor outCeiling (for uniform adjustment)
  3rd:
    - type: list
      description: inFloor inCeiling outFloor outCeiling (for by-channel adjustment red)
  4th:
    - type: list
      description: inFloor inCeiling outFloor outCeiling (for by-channel adjustment green)
  5th:
    - type: list
      description: inFloor inCeiling outFloor outCeiling (for by-channel adjustment blue)
  6th:
    - type: float
      description: low percentile (in auto mode)
  7th:
    - type: float
      description: high percentile (in auto mode)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
