---
title: hradio, hdl, radiobutton, radiobut, rdb
description: Horizontal radio
pdcategory: GUI
inlets:
  1st:
  - type: float
    description: Set and output number
outlets:
  1st:
  - type: float
    description: Selected cell number
methods:
- type: size <float>
  description: sets the GUI size
- type: orientation <float>
  description: non zero sets to vertical, zero sets to horizontal
- type: number <float>
  description: sets number of cells
- type: init <float>
  description: non zero sets to init mode.
- type: label <symbol>
  description: sets label symbol.
- type: label_font <float, float>
  description: sets font type <0: DejaVu Sans Mono, 1: Helvetica, 2: Times> and font size.
- type: label_pos <float, float>
  description: sets label position.
- type: color <list>
  description: first element sets background color and third sets font color (either floats or symbols in hexadecimal format).
- type: send <symbol>
  description: sets send symbol.
- type: receive <symbol>
  description: sets receive symbol.
- type: pos <float, float>
  description: sets position in the patch canvas.
- type: delta <float, float>
  description: changes the position by a x/y delta in pixels.

draft: false