---
title: vslider, vsl
description: vertical slider
pdcategory: vanilla, UI
inlets:
  1st:
  - type: float
    description: set and output number
outlets:
  1st:
  - type: float
    description: slider value
methods:
- type: size <float>
  description: sets the GUI size
- type: orientation <float>
  description: non-0 sets to vertical, zero sets to horizontal
- type: range <float, float>
  description: sets minimum and maximum range
- type: lin
  description: sets mode to linear
- type: log
  description: sets mode to logarithmic
- type: init <float>
  description: non-0 sets to init mode
- type: label <symbol>
  description: sets label symbol
- type: label_font <float, float>
  description: sets font type <0: DejaVu Sans Mono, 1: Helvetica, 2: Times> and font size
- type: label_pos <float, float>
  description: sets label position
- type: color <list>
  description: first element sets background color and third sets font color (either floats or symbols in hexadecimal format)
- type: send <symbol>
  description: sets send symbol
- type: receive <symbol>
  description: sets receive symbol
- type: pos <float, float>
  description: sets position in the patch canvas
- type: delta <float, float>
  description: changes the position by a x/y delta in pixels

draft: false
