---
title: forward
description: send remote messages
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: symbol
  description: initially set destination
inlets:
  1st:
  - type: anything
    description: any message to be sent forward
outlets:

methods:
  - type: send <symbol>
    description: sets an address to send to

draft: false
---

[forward] is similar to [send], but the destination can change with each message (like a semicolon in a message in Pd/Max) - it also sends to anything that a [send] does (like to an array, to "pd", etc..).

