---
title: canvas.pos

description: Report canvas position

categories:
 - object

pdcategory: Subpatch Management

arguments:
- type: float
  description: depth
  default: 0 (current window)

inlets:
  1st:
  - type: bang
    description: query for position

outlets:
  1st:
  - type: list
    description: canvas position x/y coordinates

draft: false
---

The [canvas.pos] object outputs the position of a subwindow canvas in the parent canvas. The output is the x/y coordinates. It can also query the parent status with the depth argument (1 goes up to the parent patch, 2 queries the parent's parent and so on).