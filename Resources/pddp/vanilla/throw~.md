---
title: throw~
description: throw signal to a matching catch~ object.
categories:
- object
see_also:
- send~
pdcategory: General Audio Manipulation
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: signal to throw to a matching catch~ object.
  - type: set <symbol>
    description: set throw~ name.
arguments:
- type: symbol
  description: throw~ symbol name (default empty symbol).
draft: false
---
Any number of throw~ objects can add into one catch~ object (but two catch~ objects cannot share the same name.)

NOTE: You have to get the throw~ sorted before the catch~ or else you'll never get less than a block's delay. You can use the strategy given in the example below to control this.

`../3.audio.examples/G05.execution.order`