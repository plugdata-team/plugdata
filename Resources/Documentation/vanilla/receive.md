---
title: receive, r
description: Receive messages without patch cords.
categories:
- object
pdcategory: General
last_update: '0.48'
see_also:
- send~
- send
- receive~
- samplerate~
arguments:
- description: 'receive name symbol 
  default:: empty symbol
'
  type: symbol
outlets:
  1st:
  - type: any message
    description: outputs the messages destined to its receive name.
aliases:
- r
draft: false
---
The [receive] object gets messages directly from [send] or other objects like [list store], [float], [int] and [value] via a `send` method.
