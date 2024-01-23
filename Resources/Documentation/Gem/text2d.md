---
title: text2d
description: renders a line of text without 3D transformations.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <symbol>
    description: initial text (defaults to "gem")
methods:
  - type: font <symbol>
    description: Load a TrueType-font.
  - type: text <list>
    description: Render the given text.
  - type: list <list>
    description: Render the given text.
  - type: alias <float>
    description: Anti-aliasing on/off (default: 1).
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: size (in points) (default: 20)
outlets:
  1st:
    - type: gemlist
draft: false
---
