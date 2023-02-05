---
title: canvas.gop

description: report `GOP` information

categories:
 - object

pdcategory: ELSE, UI

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
    description: GOP info <gop status, x size, y size, coords (x1, y1, x2, y1)>

draft: false
---

The [canvas.gop] object outputs the information of a "graph on parent" (aka 'GOP'), which is a subpatch with a visible graph in the parent. It can also query the GOP info with the depth argument (1 goes up to the parent patch, 2 queries the parent's parent and so on).
