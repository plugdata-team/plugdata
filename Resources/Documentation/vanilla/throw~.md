---
title: throw~
description: throw signal to a matching catch~ object
categories:
- object
see_also:
- send~
pdcategory: vanilla, Mixing and Routing, Audio I/O
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: signal to throw to a matching catch~ object
arguments:
- type: symbol
  description: throw~ symbol name 
  default: empty symbol
methods:
  - type: set <symbol>
    description: set throw~ name
draft: false
---
Any number of throw~ objects can add into one catch~ object (but two catch~ objects cannot share the same name.
