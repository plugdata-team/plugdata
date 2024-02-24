---
title: textoutline
description: renders a line of outlined text with 3D transformations
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <symbol>
    description: initial text
    default: "gem"
methods:
  - type: font <symbol>
    description: load a TrueType-font
  - type: text <list>
    description: render the given text
  - type: list <list>
    description: render the given text
  - type: justify <float> <float>
    description: horizontal & vertical justification
  - type: size <float>
    description: set the size in points
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: <float>
      description: size in points
outlets:
  1st:
    - type: gemlist
draft: false
---
