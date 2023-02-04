---
title: canvas.bounds

description: report canvas bounds

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0 - current window
inlets:
outlets:
  1st:
  - type: list
    description: bounds coordinates

draft: false
---

The [canvas.bounds] object outputs bounds coordinates in pixels, top left coordinate (horizontal, vertical) and bottom right (horizontal, vertical). Try moving the patch window or change window size to see changes.
