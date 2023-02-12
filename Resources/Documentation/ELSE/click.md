---
title: click

description: responds to clicks on parent

categories:
 - object

pdcategory: ELSE, UI, Triggers and Clocks

arguments:
- type: float
  description: sets mode, 0 is "abstraction" mode, non-0 is "subpatch" mode
  default: 0

inlets:
outlets:
  1st:
  - type: bang
    description: bang when clicked on in the parent patch

draft: false
---

[click] sends a bang when you put it in an abstraction or a subpatch and click on the parent. This is useful for making non graphical abstractions respond to a click in the same way compiled objects can. 
See below how it sends a bang when you click on the subpatch. Note that you can still open the subpatch with right click and then "open".
