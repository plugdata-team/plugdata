---
title: funnel
description: tag data based on inlet
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: sets 'n' number of inlets
  default: 2
- type: float
  description: first inlet number offset
  default: 0
inlets:
  nth:
  - type: anything
    description: any data to be tagged and output
  - type: bang
    description: sends the last received message in that inlet
outlets:
  1st:
  - type: list
    description: an incoming message preceded with the inlet number

methods:
  - type: set <anything>
    description: sets a message and doesn't output it
  - type: offset <float>
    description: sets an offset for the inlet numbers

---

[funnel] receives data from many inlets and funnels them to an outlet. The incoming data is tagged with an inlet number to be retrieved with [route] or [spray]. It can also be used to store values into a table/coll.

