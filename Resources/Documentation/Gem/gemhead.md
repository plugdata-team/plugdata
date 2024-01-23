---
title: gemhead
description: start a rendering chain
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: priority
    default: 50
methods:
  - type: set <priority>
    description: change priority value of this chain
  - type: context <name>
    description: change rendering context (for multiple windows)

inlets:
  1st:
    - type: float (1/0)
      description: turn rendering on/off
    - type: bang
      description: force rendering now
outlets:
  1st:
    - type: gemlist
draft: false
---
