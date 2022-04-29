---
title: pipe
description: dynamically growable delay line for numbers
categories:
- object
pdcategory: Time
last_update: '0.33'
see_also:
- delay
- timer
arguments:
- description: (optional) symbols sets number of inlets and type (f default,  s,  p)
    and floats set float type and initial value.
  type: list
- description: sets delay time in ms (default 0).
  order: last
  type: float
inlets:
  1st:
  - type: bang
    description: sends the last received data after the delay time.
  - type: clear
    description: forget all scheduled messages.
  - type: float/symbol/pointer
    description: the type depends on the creation argument.
  - type: flush
    description: sends the scheduled messages immediately.
  'n: number of inlets depends on creation arguments':
  - type: float/symbol/pointer
    description: the type depends on the creation argument.
  rightmost:
  - type: float
    description: set the delay time in ms.
outlets:
  'n: number of inlets depends on creation arguments':
  - type: float/symbol/pointer
    description: the type depends on the creation argument.
draft: false
---
Message "delay line".

The [pipe] object stores a sequence of messages and outputs them after a specified delay time in milliseconds. The output is scheduled when storing the incoming message. Thus changing the delay time doesn't affect the messages that are already scheduled.

You can specify the data type with a first argument (which is a float by default). A symbol argument "s", "f", or "p" specifies a "symbol", "float" (number), or pointer type (untested). The delay time comes then as the last argument.

You can specify compound messages (lists) by adding more than one argument.
