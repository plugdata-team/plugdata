---
title: vu
description: VU meter
pdcategory: vanilla, UI
inlets:
  1st:
  - type: float
    description: set RMS value in dBFS
  2nd:
  - type: float
    description: set peak value in dBFS
outlets:
  1st:
  - type: float
    description: RMS value in dBFS
  2st:
  - type: float
    description: Peak value in dBFS
methods:
- type: size <float>
  description: sets the GUI size
- type: scale <float>
  description: non-0 shows scale (default), zero hides it
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
