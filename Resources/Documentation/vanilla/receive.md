---
title: receive, r
description: receive messages without patch cords
categories:
- object
pdcategory: vanilla, UI, Mixing and Routing
last_update: '0.48'
see_also:
- send~
- send
- receive~
- samplerate~
arguments:
- description: receive name symbol 
  default: empty symbol
  type: symbol
outlets:
  1st:
  - type: anything
    description: outputs received messages
draft: false
---
The [receive] object gets messages directly from [send] or other objects like [list store], [float], [int] and [value] via a `send` method.
