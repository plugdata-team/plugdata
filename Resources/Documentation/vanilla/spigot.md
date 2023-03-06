---
title: spigot
description: pass or block messages
categories:
- object
pdcategory: vanilla, Mixing and Routing
last_update: '0.38'
arguments:
- description: initialize right inlet
  type: float
inlets:
  1st:
  - type: anything
    description: any message to pass or not
  2nd:
  - type: float
    description: non-0 to pass messages, zero to stop them
outlets:
  1st:
  - type: anything
    description: any input message if spigot is opened
draft: false
---
Spigot passes messages from its left inlet to its outlet,  as long as a non-0 number is sent to its right inlet. When its right inlet gets zero,  incoming messages are "blocked" i.e.,  ignored.
