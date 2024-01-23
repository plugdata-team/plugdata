---
title: text3d
description: renders a line of text with 3D transformations.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <symbol>
    description: initial text (defaults to "gem")
methods:
  - type: font <symbol>
    description: load a TrueType-font
  - type: text <list>
    description: render the given text
  - type: list <list>
    description: render the given text
  - type: justify <float> [<float>]
    description: horizontal & vertical justification
  - type: string <list>
    description: render the given text as a list of unicode code points (similar to ASCII)
  - type: size <float>
    description: Set the size in points (default: 20)
  - type: alias 1|0
    description: Anti-aliasing on/off (default: 1)
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: <float>
      description: size (in points) (default: 20)
outlets:
  1st:
    - type: gemlist
draft: false
---
