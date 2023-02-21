---
title: send, s
description: Send messages without patch cords
categories:
- object
pdcategory: vanilla, UI, Mixing and Routing
last_update: '0.48'
see_also:
- send~
- receive
- receive~
- samplerate~
arguments:
- description: send symbol (if given, 2nd inlet is suppressed)
  default: empty symbol
  type: symbol
inlets:
  1st:
  - type: anything
    description: sends to corresponding receive object, or named object
  2nd: (if created without arguments)
  - type: symbol
    description: sets the send name
draft: false
---
[Send] sends messages to "receive" objects. Sends and receives are named to tell them whom to connect to. They work across windows too.
