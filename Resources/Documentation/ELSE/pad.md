---
title: pad
description: mouse pad

categories:
 - object

pdcategory: ELSE, UI

arguments:

inlets:

outlets:
  1st:
  - type: list
    description: list of coordinates (x, y)
  - type: click <float>
    description: report mouse button press <1> and release <0>

flags:
  - name: -dim <f, f>
    description: set horizontal and vertical dimensions (default 127, 127)

methods:
  - type: dim <f, f>
    description: set dimension (horizontal and vertical)
  - type: width <float>
    description: set width
  - type: height <float>
    description: set height
  - type: color <f, f, f>
    description: set RGB color

draft: false
---

[pad] is a GUI object that reports mouse coordinates over its area and click status. If you click and drag, you can get values outside the GUI area.

