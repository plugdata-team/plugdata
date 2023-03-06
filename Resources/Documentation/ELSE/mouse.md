---
title: mouse

description: grab mouse

categories:
 - object

pdcategory: ELSE, Networking

arguments:

inlets:
  1st:
  - type: zero
    description: sets coordinates to zero
  - type: reset
    description: resets mouse coordinates

outlets:
  1st:
  - type: float
    description: mouse click status (1: on/ 0: off)
  2nd:
  - type: float
    description: horizontal coordinate
  3rd:
  - type: float
    description: vertical coordinate

draft: false
---

[mouse] grabs mouse interaction (click and coordinates). Click and see. This works for any click Pd receives in any window, not only this patch's.
