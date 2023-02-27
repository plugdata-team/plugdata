---
title: canvas.active

description: report window activity

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0 - current window
flags:
  - name: -name
    description: sets to name mode
inlets:
outlets:
  1st:
  - type: float
    description: window activity (0 inactive / 1 active)

draft: false
---

The [canvas.active] object outputs activity status. It sends 1 if the window is active (when it's the front-most window) and 0 when inactive. It can also query the parent status with the depth argument (1 goes up to the parent patch, 2 queries the parent's parent and so on).
