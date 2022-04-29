---
title: send
description: Send messages without patch cords.
categories:
- object
pdcategory: General
last_update: '0.48'
see_also:
- send~
- receive
- receive~
- samplerate~
arguments:
- description: 'send symbol (if given,  2nd inlet is suppressed,  default: empty symbol)'
  type: symbol
inlets:
  1st:
  - type: any message
    description: 'sends to the corresponding receive object,  or any named object
      which name corresponds to the stored symbol. e.g: array,  value,  iemguis,  directly
      to a named patch,  etc...'
  '2nd: (if created without arguments)':
  - type: symbol
    description: sets the send name.
aliases:
- s
draft: false
---
[Send] sends messages to "receive" objects. Sends and receives are named to tell them whom to connect to. They work across windows too.
