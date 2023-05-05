---
title: canvas.mouse

description: canvas' mouse interaction

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0
- type: float
  description: non-0 sets "position mode"
  default: 0
- type: float
description: non-0 allows output in edit mode
default: 0
    
inlets:
  1st:
  - type: zero
    description: sets coordinates to zero
  - type: reset
    description: resets mouse coordinates

outlets:
  1st:
  - type: float
    description: mouse click status (1=on, 0=off)
  2nd:
  - type: float
    description: horizontal coordinate
  3rd:
  - type: float 
    description: vertical coordinate

draft: false
---

[canvas.mouse] gets mouse click and mouse coordinates when your mouse is interacting with the canvas window. An optional argument sets the depth level. This object doesn't output anything if in edit mode.
