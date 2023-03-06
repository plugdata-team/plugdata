---
title: buddy
description: sync input messages
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets the 'n' number of inlets/outlets
  default: 2
inlets:
  nth:
  - type: anything
    description: any message type to be synced
  - type: bang
    description: same as sending a 0 to the inlet
outlets:
  nth:
  - type: anything
    description: the synced message

methods:
  - type: anything
    description: clears all received messages

draft: false
---

[buddy] synchronizes arriving data and outputs them only if messages have been sent to all inlets. The output is in the usual right to left order, and all input is cleared after that.

