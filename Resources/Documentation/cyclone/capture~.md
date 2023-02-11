---
title: capture~
description: store samples
categories:
 - object
pdcategory: cyclone, Buffers, Analysis
arguments:
- type: symbol
  description: "f" - collect data for the specified number of samples
- type: float
  description: number of samples to store
  default: 4096
- type: float
  description: decimal precision
  default: 4
- type: list
  description: up to 10 number indices within a signal block. If no list is given, entire block is stored
inlets:
  1st:
  - type: signal
    description: signal to be stored in a text format
outlets:

methods:
  - type: clear
    description: clear stored values and starts capturing again
  - type: write <symbol>
    description: save to file (no symbol opens dialog box)
  - type: open
    description: opens text window (like double clicking)
  - type: wclose
    description: closes text window

---

[capture~] is designed for signal debugging/investigation, it captures samples into a text window or text file.

