---
title: capture
description: store data
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: number of stored values
  default: 512
- type: symbol
  description: no arg: as decimal, "x": hex, "m": < 128 — decimal / > — hex, "a": only symbols
inlets:
  1st:
  - type: anything
    description: stores items (float/symbol) sequentially
outlets:
  1st:
  - type: float/symbol
    description: stored items in sequential order (on dump message)
  2nd:
  - type: float
    description: number of items received since last 'count' message

flags:
  - name: @precision <float>
    description: decimal precision for floats (default 4)

methods:
  - type: clear
    description: clears stored contents
  - type: count
    description: outputs number of received items since last 'count' message
  - type: dump
    description: outputs stored elements sequentially (first in first out)
  - type: open
    description: opens editing window, same as clicking on it
  - type: wclose
    description: closes editing window
  - type: write <symbol>
    description: saves to a file (no symbol opens dialog box)

draft: false
---

[capture] stores items in the received order for viewing, editing, and saving to a file. If the maximum number of items is exceeded, the earliest stored item is dropped.

