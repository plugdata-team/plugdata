---
title: canvas.vis

description: report window visibility

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0 - current window

inlets:
  1st:
  - type: bang
    description: query for visibility

outlets:
  1st:
  - type: float
    description: window visibility (0 invisible / 1 visible)

draft: false
---

The [canvas.vis] object outputs visibility status. It sends 1 if the window is visible and 0 when it's not. It can also query the parent status with the depth argument (1 goes up to the parent patch, 2 queries the parent's parent and so on). The object can react to most visibility changes and you can also query for it with a bang.
