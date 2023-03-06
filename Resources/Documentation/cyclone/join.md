---
title: join
description: combine items into a list
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: number of inlets (min 2 / max 255)
  default: 2
inlets:
  nth:
  - type: anything
    description: any message type to be combined into a list
  - type: bang
    description: forces output of stored messages
outlets:
  1st:
  - type: list
    description: the list composed of the joined messages

flags:
  - name: @triggers <list>
    description: default "0" (left inlet is hot)

methods:
  - type: triggers <list>
    description: define a list of input numbers that trigger an output (make the inlet "hot"): 0 is the first inlet, 1 the second and so on (-1 makes all inlets "hot")
  - type: set <anything>
    description: sets a message to that inlet without output

draft: false
---

[join] takes any type of messages (anything, float, symbol, list) in any inlet and combines them all into an output list.

