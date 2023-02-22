---
title: pic
description: load pictures

categories:
 - object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: open <symbol>
    description: open an image file named by the symbol

outlets:
  1st:
  - type: bang
    description: a bang is output if you click on the image
  - type: float
    description: 1/0 when in latch mode
  - type: list
    description: pic width/height when in report size mode

flags:
  - name: -open <symbol>
    description: sets file name to open (default 'empty')
  - name: -send <symbol>
    description: sets send symbol (default 'empty')
  - name: -receive <symbol>
    description: sets receive symbol (default 'empty')
  - name: -outline
    description: sets to outline mode (default no outline)
  - name: -size
    description: sets to "report size" mode (default no report)
  - name: -latch
    description: sets to "latch" mode (default "bang" mode)

methods:
  - type: open <symbol>
    description: open an image file named by the symbol
  - type: set <symbol>
    description: same as open, but pd doesn't ask to save changes
  - type: latch <float>
    description: non-0 sets to latch mode
  - type: outline <float>
    description: non-0 sets to outline mode
  - type: size <float>
    description: non-0 sets to report size mode
  - type: send <symbol>
    description: sets a send symbol
  - type: receive <symbol>
    description: sets a receive symbol
draft: false
---

[pic] loads image pictures that you can interact with. It only works with file types: .gif, .ppm & .pgm. you click on the picture and it sends a bang (default) or 1 when clicking and 0 when releasing (when in latch mode).

