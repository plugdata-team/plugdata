---
title: spray

description: distribute values to outlets

categories:
 - object

pdcategory: cyclone, Mixing and Routing, Data Management

arguments:
- type: float
  description: sets the number of outlets
  default: 2
- type: float
  description: outlet offset number
  default: 0
- type: float
  description: non-0 — output list mode
  default: 0

inlets:
  1st:
  - type: list
    description: outlet index and elements to be distributed

outlets:
  nth:
  - type: float/symbol
    description: element sent from an inlet to a specified outlet
  - type: list
    description: when in "list output" mode


draft: true
---

[spray] accepts lists where the 1st value indicates the outlet number (starting at 0) and the 2nd element (float or symbol) is sent to that outlet - when not in list mode, subsequent elements are sent to subsequent outlets to the right, other wise (in list mode) the whole list is output at that outlet.

