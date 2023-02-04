---
title: switch

description: pass messages from a specified inlet

categories:
 - object

pdcategory: cyclone, Mixing and Routing, Data Management

arguments:
- type: float
  description: set the 'n' number of inlets (from 2 to 100)
  default: 2
- type: float
  description: inlet initially switched on 
  default: 0 (all closed)

inlets:
  1st:
  - type: float
    description: sets which inlet is open (0 — all closed)
  - type: bang
    description: outputs the open outlet number
  nth:
  - type: anything
    description: any message to pass through the switch

outlets:
  1st:
  - type: anything
    description: message from the switched on inlet

draft: true
---

[switch] outputs data from the inlet that's "switched on". Just one inlet from 'n' inlets can send data, or none of them.
