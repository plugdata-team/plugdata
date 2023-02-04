---
title: capture~
description: store samples
categories:
 - object
pdcategory: cyclone, Buffers, Analysis
arguments:
- type: symbol
  description: optional flag "f" for "first mode" - where collecting data stops after receiving the specified number of samples. If not given, the default is "last mode", where it continues to collect data, throwing away old values if it has received more than the specified samples
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

